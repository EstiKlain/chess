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

// One entry per connection currently bound to the session. id is the
// connectionId - a stable, player-count-agnostic identity - kept separate
// from color, which stays the domain-meaningful field (and a plain string,
// not a fixed wire-enum, so it can grow past 'w'/'b' in future variants
// without breaking this DTO's shape). Identical for every recipient of a
// given STATE_UPDATE - only the top-level "role" field below varies.
struct PlayerDto {
    std::string id;
    std::string color;
    std::string name;
};

struct StateUpdateDto {
    int rows = 0;
    int cols = 0;
    std::vector<PieceDto> pieces;
    bool gameOver = false;
    long nowMs = 0;
    char role = ' ';  // the color this specific recipient plays, 'w' or 'b'
    std::vector<PlayerDto> players;
    std::optional<std::string> winner;  // "w"/"b", present only when gameOver
    std::optional<std::string> reason;  // "king_captured"/"resignation", present only when gameOver
};

/// Serializes a PlayerDto to JSON.
inline void to_json(nlohmann::json& j, const PlayerDto& d) {
    j = nlohmann::json{{"id", d.id}, {"color", d.color}, {"name", d.name}};
}

/// Parses a PlayerDto out of a STATE_UPDATE payload's "players" array. Needed client-side (ServerConnection) to read the roster back; the server itself only ever serializes this DTO, never parses it.
inline void from_json(const nlohmann::json& j, PlayerDto& d) {
    d.id = j.at("id").get<std::string>();
    d.color = j.at("color").get<std::string>();
    d.name = j.at("name").get<std::string>();
}

/// Serializes a MotionDto to JSON.
inline void to_json(nlohmann::json& j, const MotionDto& d) {
    j = nlohmann::json{
        {"fromRow", d.fromRow}, {"fromCol", d.fromCol},
        {"toRow", d.toRow},     {"toCol", d.toCol},
        {"startMs", d.startMs}, {"durationMs", d.durationMs},
    };
}

/// Parses a MotionDto out of a piece's "motion" object.
inline void from_json(const nlohmann::json& j, MotionDto& d) {
    d.fromRow = j.at("fromRow").get<int>();
    d.fromCol = j.at("fromCol").get<int>();
    d.toRow = j.at("toRow").get<int>();
    d.toCol = j.at("toCol").get<int>();
    d.startMs = j.at("startMs").get<long>();
    d.durationMs = j.at("durationMs").get<long>();
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

/// Parses a PieceDto out of a STATE_UPDATE payload's "pieces" array. "motion" is only present while the piece is actually moving/jumping - absent otherwise, matching to_json's own conditional field.
inline void from_json(const nlohmann::json& j, PieceDto& d) {
    d.id = j.at("id").get<int>();
    d.color = j.at("color").get<std::string>().at(0);
    d.kind = j.at("kind").get<std::string>().at(0);
    d.row = j.at("row").get<int>();
    d.col = j.at("col").get<int>();
    d.state = j.at("state").get<std::string>();
    d.stateStartMs = j.at("stateStartMs").get<long>();
    d.stateDurationMs = j.at("stateDurationMs").get<long>();
    if (j.contains("motion")) {
        d.motion = j.at("motion").get<MotionDto>();
    }
}

/// Serializes a full StateUpdateDto (snapshot + per-recipient role + player roster) to JSON. winner/reason are only present when gameOver, matching PieceDto::motion's existing conditional-field pattern.
inline void to_json(nlohmann::json& j, const StateUpdateDto& d) {
    j = nlohmann::json{
        {"rows", d.rows}, {"cols", d.cols}, {"pieces", d.pieces},
        {"gameOver", d.gameOver}, {"nowMs", d.nowMs},
        {"role", std::string(1, d.role)},
        {"players", d.players},
    };
    if (d.winner.has_value()) {
        j["winner"] = *d.winner;
    }
    if (d.reason.has_value()) {
        j["reason"] = *d.reason;
    }
}

/// Parses a full StateUpdateDto out of a STATE_UPDATE message's payload JSON - the reverse of to_json above, needed client-side (ServerConnection) to turn a received STATE_UPDATE back into data GameSnapshotMapper::fromDto can convert to a domain GameSnapshot.
inline void from_json(const nlohmann::json& j, StateUpdateDto& d) {
    d.rows = j.at("rows").get<int>();
    d.cols = j.at("cols").get<int>();
    d.pieces = j.at("pieces").get<std::vector<PieceDto>>();
    d.gameOver = j.at("gameOver").get<bool>();
    d.nowMs = j.at("nowMs").get<long>();
    d.role = j.at("role").get<std::string>().at(0);
    d.players = j.at("players").get<std::vector<PlayerDto>>();
    if (j.contains("winner")) {
        d.winner = j.at("winner").get<std::string>();
    }
    if (j.contains("reason")) {
        d.reason = j.at("reason").get<std::string>();
    }
}
