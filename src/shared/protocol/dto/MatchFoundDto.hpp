#pragma once

#include <string>

#include <nlohmann/json.hpp>

// MATCH_FOUND payload: { "sessionToken", "color", "opponent": { "username", "elo" } }.
// sessionToken lets the matched player RECONNECT if they drop mid-game.
struct OpponentDto {
    std::string username;
    int elo = 0;
};

struct MatchFoundDto {
    std::string sessionToken;
    char color = ' ';
    OpponentDto opponent;
};

/// Serializes an OpponentDto to JSON.
inline void to_json(nlohmann::json& j, const OpponentDto& d) {
    j = nlohmann::json{{"username", d.username}, {"elo", d.elo}};
}

/// Serializes a MatchFoundDto to JSON. No from_json yet - added once the client stage needs to parse this.
inline void to_json(nlohmann::json& j, const MatchFoundDto& d) {
    j = nlohmann::json{{"sessionToken", d.sessionToken}, {"color", std::string(1, d.color)}, {"opponent", d.opponent}};
}
