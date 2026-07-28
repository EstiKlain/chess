#pragma once

#include <nlohmann/json.hpp>

// JUMP payload: { "row", "col" } - a single board position, deliberately NOT
// shaped like MoveDto's {from, to} pair. GameEngine::requestJump(int row, int
// col) (src/core/engine/GameEngine.hpp) takes one position, not a move
// between two, so JUMP is not "MOVE with the same shape" - it needs its own
// DTO and its own mapper.
struct JumpDto {
    int row = 0;
    int col = 0;
};

/// Serializes a JumpDto to its wire JSON representation.
inline void to_json(nlohmann::json& j, const JumpDto& d) {
    j = nlohmann::json{
        {"row", d.row},
        {"col", d.col},
    };
}

/// Parses a JumpDto out of a JUMP message's payload JSON.
inline void from_json(const nlohmann::json& j, JumpDto& d) {
    d.row = j.at("row").get<int>();
    d.col = j.at("col").get<int>();
}
