#pragma once

#include <string>

#include <nlohmann/json.hpp>

// LOGIN payload: { "username", "password" }. Reusing MoveDto/JumpDto's
// to_json/from_json pattern.
struct LoginDto {
    std::string username;
    std::string password;
};

/// Serializes a LoginDto to its wire JSON representation.
inline void to_json(nlohmann::json& j, const LoginDto& d) {
    j = nlohmann::json{{"username", d.username}, {"password", d.password}};
}

/// Parses a LoginDto out of a LOGIN message's payload JSON.
inline void from_json(const nlohmann::json& j, LoginDto& d) {
    d.username = j.at("username").get<std::string>();
    d.password = j.at("password").get<std::string>();
}
