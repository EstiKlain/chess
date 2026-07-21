#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "server/application/ConnectionManager.hpp"
#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/ITransport.hpp"

class MakeMoveUseCase {
public:
    MakeMoveUseCase(IEventBus& bus, ITransport& transport, ConnectionManager& connections);

    /// Handles a MOVE event: maps the payload, applies it under the session's lock, and fans out the result.
    void handleMove(const std::string& connectionId, const nlohmann::json& payload);

    /// Handles a JUMP event: same flow as handleMove, but for GameEngine::requestJump's single-position shape.
    void handleJump(const std::string& connectionId, const nlohmann::json& payload);

private:
    /// Sends STATE_UPDATE (with each recipient's own role) to every connection bound to a session, and publishes MoveApplied on the bus. Named "fanOut", not "broadcast", because it deliberately does NOT use ITransport::broadcast() - it sends one distinct message per recipient (each with its own role).
    void fanOutStateUpdate(const std::string& connectionId, GameSession& session);

    /// Sends an ERROR envelope with the given code/message to one connection only (the request's sender).
    void sendError(const std::string& connectionId, const std::string& code, const std::string& message);

    IEventBus& bus_;
    ITransport& transport_;
    ConnectionManager& connections_;
};
