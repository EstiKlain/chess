#pragma once

#include <nlohmann/json.hpp>

// MOVE payload: { "fromRow", "fromCol", "toRow", "toCol" } - raw board
// coordinates, matching core/'s own Position representation. There is no
// algebraic ("e2") notation anywhere in core/ or view/ (BoardMapper converts
// pixels straight to numeric Position), so the wire format uses the same
// numeric coordinates instead of inventing a translation layer nothing else
// produces or consumes.
struct MoveDto {
    int fromRow = 0;
    int fromCol = 0;
    int toRow = 0;
    int toCol = 0;
};

/// Serializes a MoveDto to its wire JSON representation.
inline void to_json(nlohmann::json& j, const MoveDto& d) {
    j = nlohmann::json{
        {"fromRow", d.fromRow},
        {"fromCol", d.fromCol},
        {"toRow", d.toRow},
        {"toCol", d.toCol},
    };
}

/// Parses a MoveDto out of a MOVE message's payload JSON.
inline void from_json(const nlohmann::json& j, MoveDto& d) {
    d.fromRow = j.at("fromRow").get<int>();
    d.fromCol = j.at("fromCol").get<int>();
    d.toRow = j.at("toRow").get<int>();
    d.toCol = j.at("toCol").get<int>();
}
