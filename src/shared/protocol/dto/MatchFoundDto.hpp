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

/// Parses an OpponentDto out of MATCH_FOUND's "opponent" field.
inline void from_json(const nlohmann::json& j, OpponentDto& d) {
    d.username = j.at("username").get<std::string>();
    d.elo = j.at("elo").get<int>();
}

/// Serializes a MatchFoundDto to JSON.
inline void to_json(nlohmann::json& j, const MatchFoundDto& d) {
    j = nlohmann::json{{"sessionToken", d.sessionToken}, {"color", std::string(1, d.color)}, {"opponent", d.opponent}};
}

/// Parses a MatchFoundDto out of a MATCH_FOUND message's payload JSON - needed client-side (ServerConnection).
inline void from_json(const nlohmann::json& j, MatchFoundDto& d) {
    d.sessionToken = j.at("sessionToken").get<std::string>();
    d.color = j.at("color").get<std::string>().at(0);
    d.opponent = j.at("opponent").get<OpponentDto>();
}
