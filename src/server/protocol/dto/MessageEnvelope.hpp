#pragma once

#include <string>

#include <nlohmann/json.hpp>

// The one uniform envelope every message uses, in both directions:
//   { "type": "MOVE", "requestId": "uuid", "payload": { "from": "e2", "to": "e5" } }
//
// This struct, plus WebSocketTransport, are the only two places in the
// server that know a raw JSON string is involved at all. Everything past
// MessageRouter deals in BusEvent / typed DTOs, not JSON text.
struct MessageEnvelope {
    std::string type;
    std::string requestId;
    nlohmann::json payload = nlohmann::json::object();
};

inline void to_json(nlohmann::json& j, const MessageEnvelope& e) {
    j = nlohmann::json{
        {"type", e.type},
        {"requestId", e.requestId},
        {"payload", e.payload},
    };
}

inline void from_json(const nlohmann::json& j, MessageEnvelope& e) {
    e.type = j.at("type").get<std::string>();
    e.requestId = j.value("requestId", std::string{});
    e.payload = j.value("payload", nlohmann::json::object());
}
