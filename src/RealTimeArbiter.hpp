#pragma once

#include <string>
#include <vector>

#include "Board.hpp"
#include "Moves.hpp"
#include "PieceRules.hpp"

// Owns every motion/jump currently in progress. This is exactly the
// data that used to live as GameState::activeMoves / activeJumps -
// it only moved house (Stage 2); the resolution logic itself
// (resolveMoves body) is unchanged from before the refactor.
class RealTimeArbiter
{
public:
    // Advances resolution: lands/captures/promotes any move whose time is
    // up, and drops any jump that has landed. Mutates the board in place.
    // Returns the tokens captured this call (used by GameEngine to check
    // for a king capture / game over).
    std::vector<std::string> resolveMoves(Board &board, long elapsedMs, const pieceRules::PieceRulesRegistry &registry);

    bool isPieceInFlight(int row, int col) const;

    bool hasActiveMotion() const;

    // Registers a validated move as in-flight. Called by GameEngine only
    // after RuleEngine has already accepted the move.
    void startMotion(const PieceMove &move);

    // Jump-side equivalents, needed because sendJump's logic is moving
    // into GameEngine::requestJump (see Stage 7 note) and it depended on
    // this same private state.
    bool hasActiveJumpAt(int row, int col) const;
    void startJump(const JumpMove &jump);

private:
    std::vector<PieceMove> activeMoves_;
    std::vector<JumpMove> activeJumps_;
};
