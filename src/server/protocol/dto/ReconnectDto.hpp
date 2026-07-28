#pragma once

#include <string>

#include <nlohmann/json.hpp>

// RECONNECT payload: { sessionToken }, client->server only - the server
// never sends one, only parses it, mirroring LoginDto's asymmetry (to_json
// only) in the other direction.
struct ReconnectDto {
    std::string sessionToken;
};

/// Parses a ReconnectDto out of a RECONNECT message's payload JSON.
inline void from_json(const nlohmann::json& j, ReconnectDto& d) {
    d.sessionToken = j.at("sessionToken").get<std::string>();
}
