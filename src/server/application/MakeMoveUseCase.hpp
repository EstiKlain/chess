#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "server/application/ConnectionManager.hpp"
#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/ITransport.hpp"

// Real MOVE/JUMP handling, kept out of main_server.cpp so the composition
// root stays wiring-only (the fix for the Iteration 1 note about business
// logic sitting in a bus.subscribe lambda). Depends only on core/ +
// domain_ports/ + protocol/ mappers, never on WebSocketTransport/
// InProcessEventBus directly.
class MakeMoveUseCase {
public:
    MakeMoveUseCase(IEventBus& bus, ITransport& transport, ConnectionManager& connections);

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

    /// Sends STATE_UPDATE (with each recipient's own role) to every connection bound to a session, and publishes MoveApplied on the bus. Named "fanOut", not "broadcast", because it deliberately does NOT use ITransport::broadcast() - it sends one distinct message per recipient (each with its own role). Only the mover's own connectionId gets requestId echoed back; every other recipient gets "" - they never sent this request.
    void fanOutStateUpdate(const std::string& connectionId, const std::string& requestId, GameSession& session);

    /// Sends an ERROR envelope with the given code/message to one connection only (the request's sender), correlated to its requestId.
    void sendError(const std::string& connectionId, const std::string& requestId, const std::string& code,
                    const std::string& message);

    IEventBus& bus_;
    ITransport& transport_;
    ConnectionManager& connections_;
};
