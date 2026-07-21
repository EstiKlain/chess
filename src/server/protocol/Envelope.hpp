#pragma once

#include <string>

#include <nlohmann/json.hpp>

// The `{ "type", "requestId", "payload" }` triple is the one shape every
// outgoing message shares (PONG, STATE_UPDATE, ERROR, ...) - it belongs to
// the wire protocol itself, not to any one DTO, so it gets its own shared
// builder instead of being hand-rolled at each call site (which is exactly
// what happened before this file existed: PONG and STATE_UPDATE each built
// their own copy of the same three keys).
namespace protocol {

/// Wraps a payload in the standard `{ "type", "requestId", "payload" }` envelope as a JSON string, ready to pass to ITransport::send.
inline std::string envelope(const std::string& type, const nlohmann::json& payload) {
    const nlohmann::json message = {
        {"type", type},
        {"requestId", ""},
        {"payload", payload},
    };
    return message.dump();
}

/// Convenience wrapper for the one payload shape used often enough to name: an ERROR envelope with a `{ "code", "message" }` payload.
inline std::string errorEnvelope(const std::string& code, const std::string& message) {
    return envelope("ERROR", {{"code", code}, {"message", message}});
}

}  // namespace protocol
