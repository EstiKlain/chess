#pragma once

#include <optional>
#include <vector>

#include "realtime/Moves.hpp"
#include "rules/PieceRules.hpp"
#include "model/Board.hpp"

// Owns every motion/jump currently in progress. The resolution logic
// itself (resolveMoves body) is unchanged in *behavior* from before
// the Piece/Board refactor - only the data it reads/writes moved from
// string tokens on a grid to Piece objects owned by Board.
class RealTimeArbiter
{
public:
    // Advances resolution: lands/captures/promotes any move whose time is
    // up, and drops any jump that has landed. Mutates the board in place.
    // Returns snapshots of the pieces captured this call (used by
    // GameEngine to check for a king capture / game over).
    std::vector<Piece> resolveMoves(Board &board, long elapsedMs, const pieceRules::PieceRulesRegistry &registry);

    bool isPieceInFlight(int row, int col) const;

    bool hasActiveMotion() const;

    // Registers a validated move as in-flight and marks the piece as
    // Moving. Called by GameEngine only after RuleEngine has already
    // accepted the move.
    void startMotion(Board &board, const PieceMove &move);

    bool hasActiveJumpAt(int row, int col) const;
    void startJump(Board &board, const JumpMove &jump);

    std::optional<PieceMove> activeMoveForPiece(int pieceId) const;
    std::optional<JumpMove> activeJumpForPiece(int pieceId) const;

    bool isPieceResting(int pieceId) const;
    std::optional<RestWindow> activeRestForPiece(int pieceId) const;

private:
    std::vector<PieceMove> activeMoves_;
    std::vector<JumpMove> activeJumps_;
    std::vector<RestWindow> activeRests_;

    void startRest(Board &board, int pieceId, long startMs, long durationMs, PieceState kind); // NEW
};
