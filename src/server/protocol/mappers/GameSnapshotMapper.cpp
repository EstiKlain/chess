#include "server/protocol/mappers/GameSnapshotMapper.hpp"

#include "server/protocol/dto/StateUpdateDto.hpp"

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

    dto.pieces.reserve(snapshot.pieces.size());
    for (const PieceSnapshot& piece : snapshot.pieces) {
        dto.pieces.push_back(toPieceDto(piece));
    }

    return nlohmann::json(dto);
}

}  // namespace GameSnapshotMapper
