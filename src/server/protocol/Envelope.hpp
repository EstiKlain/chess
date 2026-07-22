#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "server/domain_ports/ITransport.hpp"

namespace protocol {

/// Wraps a payload in the standard `{ "type", "requestId", "payload" }` envelope as a JSON string, ready to pass to ITransport::send. requestId correlates this message with the specific request that caused it - pass "" for messages with no originating request from this recipient (e.g. a bystander's STATE_UPDATE, or PONG).
inline std::string envelope(const std::string& type, const std::string& requestId, const nlohmann::json& payload) {
    const nlohmann::json message = {
        {"type", type},
        {"requestId", requestId},
        {"payload", payload},
    };
    return message.dump();
}

/// Convenience overload for the common case of no correlation (PONG, and every other call site until this was requestId-aware).
inline std::string envelope(const std::string& type, const nlohmann::json& payload) {
    return envelope(type, "", payload);
}

/// Convenience wrapper for the one payload shape used often enough to name: an ERROR envelope with a `{ "code", "message" }` payload, correlated to the request that caused it.
inline std::string errorEnvelope(const std::string& requestId, const std::string& code, const std::string& message) {
    return envelope("ERROR", requestId, {{"code", code}, {"message", message}});
}

/// Overload for ERROR envelopes with no originating request to correlate to (e.g. MessageRouter's envelope-level parse failure, where requestId couldn't even be read).
inline std::string errorEnvelope(const std::string& code, const std::string& message) {
    return errorEnvelope("", code, message);
}

/// Sends an ERROR envelope to one connection only, correlated to its requestId. Introduced to avoid LoginUseCase, AuthGuard, and MakeMoveUseCase each independently duplicating this same "build an error envelope, send it" pair.
inline void sendError(ITransport& transport, const std::string& connectionId, const std::string& requestId,
                       const std::string& code, const std::string& message) {
    transport.send(connectionId, errorEnvelope(requestId, code, message));
}

}  // namespace protocol
