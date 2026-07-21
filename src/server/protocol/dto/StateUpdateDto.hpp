#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// STATE_UPDATE payload: a JSON mirror of core::GameSnapshot, plus a "role"
// field. role is per-recipient (each connection in a session sees its own
// color), never shared between the two STATE_UPDATE sends for one move -
// see GameSnapshotMapper::toJson.
struct MotionDto {
    int fromRow = 0;
    int fromCol = 0;
    int toRow = 0;
    int toCol = 0;
    long startMs = 0;
    long durationMs = 0;
};

struct PieceDto {
    int id = 0;
    char color = ' ';
    char kind = ' ';
    int row = 0;
    int col = 0;
    std::string state;  // "Idle"/"Moving"/"Jumping"/"RestingShort"/"RestingLong"/"Captured"
    long stateStartMs = 0;
    long stateDurationMs = 0;
    std::optional<MotionDto> motion;
};

struct StateUpdateDto {
    int rows = 0;
    int cols = 0;
    std::vector<PieceDto> pieces;
    bool gameOver = false;
    long nowMs = 0;
    char role = ' ';  // the color this specific recipient plays, 'w' or 'b'
};

/// Serializes a MotionDto to JSON.
inline void to_json(nlohmann::json& j, const MotionDto& d) {
    j = nlohmann::json{
        {"fromRow", d.fromRow}, {"fromCol", d.fromCol},
        {"toRow", d.toRow},     {"toCol", d.toCol},
        {"startMs", d.startMs}, {"durationMs", d.durationMs},
    };
}

/// Serializes a PieceDto to JSON.
inline void to_json(nlohmann::json& j, const PieceDto& d) {
    j = nlohmann::json{
        {"id", d.id},   {"color", std::string(1, d.color)},
        {"kind", std::string(1, d.kind)}, {"row", d.row}, {"col", d.col},
        {"state", d.state},
        {"stateStartMs", d.stateStartMs}, {"stateDurationMs", d.stateDurationMs},
    };
    if (d.motion.has_value()) {
        j["motion"] = *d.motion;
    }
}

/// Serializes a full StateUpdateDto (snapshot + per-recipient role) to JSON.
inline void to_json(nlohmann::json& j, const StateUpdateDto& d) {
    j = nlohmann::json{
        {"rows", d.rows}, {"cols", d.cols}, {"pieces", d.pieces},
        {"gameOver", d.gameOver}, {"nowMs", d.nowMs},
        {"role", std::string(1, d.role)},
    };
}
