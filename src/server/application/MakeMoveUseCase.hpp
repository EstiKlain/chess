#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "server/application/ConnectionManager.hpp"
#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

class MakeMoveUseCase {
public:
    MakeMoveUseCase(IEventBus& bus, ITransport& transport, ConnectionManager& connections, IIdentityStore& identities);

    /// Handles a MOVE event: maps the payload, applies it under the session's lock, and fans out the result.
    void handleMove(const std::string& connectionId, const std::string& requestId, const nlohmann::json& payload);

    /// Handles a JUMP event: same flow as handleMove, but for GameEngine::requestJump's single-position shape.
    void handleJump(const std::string& connectionId, const std::string& requestId, const nlohmann::json& payload);

private:
    /// Looks up the GameSession bound to connectionId; sends INTERNAL_ERROR and returns nullopt if none is bound (should never happen in practice - rejection already happened at connect time).
    std::optional<GameSession*> lookupBoundSession(const std::string& connectionId, const std::string& requestId);

    /// Shared tail of handleMove/handleJump: MoveResult and JumpResult have the identical {accepted, reason} shape, so this takes the two fields directly rather than being templated on the result type.
    void finishRequest(const std::string& connectionId, const std::string& requestId, GameSession& session,
                        bool accepted, const std::string& reason);

    /// Sends STATE_UPDATE (with each recipient's own role, and the shared player roster) to every connection bound to a session, and publishes MoveApplied on the bus. Named "fanOut", not "broadcast", because it deliberately does NOT use ITransport::broadcast() - it sends one distinct message per recipient (each with its own role). Only the mover's own connectionId gets requestId echoed back; every other recipient gets "" - they never sent this request.
    void fanOutStateUpdate(const std::string& connectionId, const std::string& requestId, GameSession& session);

    IEventBus& bus_;
    ITransport& transport_;
    ConnectionManager& connections_;
    IIdentityStore& identities_;
};
