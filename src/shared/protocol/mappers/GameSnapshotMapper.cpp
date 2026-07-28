#include "shared/protocol/mappers/GameSnapshotMapper.hpp"

#include "shared/protocol/dto/StateUpdateDto.hpp"

namespace {

std::string stateToString(PieceState state) {
    switch (state) {
        case PieceState::Idle: return "Idle";
        case PieceState::Moving: return "Moving";
        case PieceState::Jumping: return "Jumping";
        case PieceState::RestingShort: return "RestingShort";
        case PieceState::RestingLong: return "RestingLong";
        case PieceState::Captured: return "Captured";
    }
    return "Idle";
}

PieceDto toPieceDto(const PieceSnapshot& piece) {
    PieceDto dto;
    dto.id = piece.id;
    dto.color = piece.color;
    dto.kind = piece.kind;
    dto.row = piece.row;
    dto.col = piece.col;
    dto.state = stateToString(piece.state);
    dto.stateStartMs = piece.stateStartMs;
    dto.stateDurationMs = piece.stateDurationMs;
    if (piece.motion.has_value()) {
        const MotionSnapshot& motion = *piece.motion;
        dto.motion = MotionDto{motion.fromRow,  motion.fromCol, motion.toRow,
                                motion.toCol,    motion.startMs, motion.durationMs};
    }
    return dto;
}

std::string reasonToString(GameOverReason reason) {
    switch (reason) {
        case GameOverReason::KingCaptured: return "king_captured";
        case GameOverReason::Resignation: return "resignation";
    }
    return "king_captured";
}

GameOverReason reasonFromString(const std::string& reason) {
    return (reason == "resignation") ? GameOverReason::Resignation : GameOverReason::KingCaptured;
}

PieceState stateFromString(const std::string& state) {
    if (state == "Idle") return PieceState::Idle;
    if (state == "Moving") return PieceState::Moving;
    if (state == "Jumping") return PieceState::Jumping;
    if (state == "RestingShort") return PieceState::RestingShort;
    if (state == "RestingLong") return PieceState::RestingLong;
    if (state == "Captured") return PieceState::Captured;
    return PieceState::Idle;
}

PieceSnapshot fromPieceDto(const PieceDto& dto) {
    PieceSnapshot piece{};
    piece.id = dto.id;
    piece.color = dto.color;
    piece.kind = dto.kind;
    piece.row = dto.row;
    piece.col = dto.col;
    piece.state = stateFromString(dto.state);
    piece.stateStartMs = dto.stateStartMs;
    piece.stateDurationMs = dto.stateDurationMs;
    if (dto.motion.has_value()) {
        const MotionDto& motion = *dto.motion;
        piece.motion = MotionSnapshot{motion.fromRow,  motion.fromCol, motion.toRow,
                                       motion.toCol,    motion.startMs, motion.durationMs};
    }
    return piece;
}

}  // namespace

namespace GameSnapshotMapper {

nlohmann::json toJson(const GameSnapshot& snapshot, char recipientColor, const std::vector<PlayerDto>& players) {
    StateUpdateDto dto;
    dto.rows = snapshot.rows;
    dto.cols = snapshot.cols;
    dto.gameOver = snapshot.gameOver;
    dto.nowMs = snapshot.nowMs;
    dto.role = recipientColor;
    dto.players = players;
    if (snapshot.winner.has_value()) {
        dto.winner = std::string(1, *snapshot.winner);
    }
    if (snapshot.gameOverReason.has_value()) {
        dto.reason = reasonToString(*snapshot.gameOverReason);
    }

    dto.pieces.reserve(snapshot.pieces.size());
    for (const PieceSnapshot& piece : snapshot.pieces) {
        dto.pieces.push_back(toPieceDto(piece));
    }

    return nlohmann::json(dto);
}

GameSnapshot fromDto(const StateUpdateDto& dto) {
    GameSnapshot snapshot;
    snapshot.rows = dto.rows;
    snapshot.cols = dto.cols;
    snapshot.gameOver = dto.gameOver;
    snapshot.nowMs = dto.nowMs;
    if (dto.winner.has_value()) {
        snapshot.winner = dto.winner->at(0);
    }
    if (dto.reason.has_value()) {
        snapshot.gameOverReason = reasonFromString(*dto.reason);
    }

    snapshot.pieces.reserve(dto.pieces.size());
    for (const PieceDto& piece : dto.pieces) {
        snapshot.pieces.push_back(fromPieceDto(piece));
    }

    return snapshot;
}

}  // namespace GameSnapshotMapper
