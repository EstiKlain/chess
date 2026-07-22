#pragma once

#include <string>

#include <nlohmann/json.hpp>

// LOGIN payload: { "username" } only - no "password" field until Iteration
// 6 adds SQLite-backed credential checking. Reusing MoveDto/JumpDto's
// to_json/from_json pattern.
struct LoginDto {
    std::string username;
};

/// Serializes a LoginDto to its wire JSON representation.
inline void to_json(nlohmann::json& j, const LoginDto& d) {
    j = nlohmann::json{{"username", d.username}};
}

/// Parses a LoginDto out of a LOGIN message's payload JSON.
inline void from_json(const nlohmann::json& j, LoginDto& d) {
    d.username = j.at("username").get<std::string>();
}
