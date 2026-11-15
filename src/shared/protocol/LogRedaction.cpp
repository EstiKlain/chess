#include "shared/protocol/LogRedaction.hpp"

#include <nlohmann/json.hpp>

namespace protocol {

std::string redactForLog(const std::string& rawJson) {
    // Cheap short-circuit before parsing - only LOGIN payloads ever carry a
    // "password" field, and STATE_UPDATE (the overwhelming majority of
    // traffic, sent every tick) never does. Avoids a full JSON parse on
    // every single message just to redact the rare few that need it.
    if (rawJson.find("\"password\"") == std::string::npos) {
        return rawJson;
    }

    nlohmann::json message;
    try {
        message = nlohmann::json::parse(rawJson);
    } catch (const nlohmann::json::exception&) {
        return rawJson;
    }

    if (message.value("type", "") != "LOGIN" || !message.contains("payload") ||
        !message["payload"].contains("password")) {
        return rawJson;
    }

    message["payload"]["password"] = "***";
    return message.dump();
}

}  // namespace protocol
