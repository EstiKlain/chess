#pragma once

#include <string>
#include <vector>

#include "Board.hpp"
#include "MoveRequest.hpp"
#include "RealTimeArbiter.hpp"
#include "PieceRules.hpp"

// Result of a move/jump request - always carries a reason, even on
// success ("legal") - so there is no more silent `return;` anywhere
// on the request path (Stage 3 goal).
struct MoveResult
{
    bool accepted;
    std::string reason;
};

struct JumpResult
{
    bool accepted;
    std::string reason;
};

// The single gate from the outside world into the game (Stage 3).
// Owns the Board, the game-over flag, the clock, and the arbiter.
// Controller and ScriptRunner/runCommands only ever call these public
// methods - they never reach into board/arbiter directly.
class GameEngine
{
public:
    GameEngine(Board board, pieceRules::PieceRulesRegistry rules)
        : board_(std::move(board)), rules_(std::move(rules)) {}

    // Fixed check order (agreed on beforehand): game_over -> motion_in_progress
    // -> RuleEngine -> startMotion. Always returns a MoveResult.
    MoveResult requestMove(const MoveRequest &request);

    // Symmetric to requestMove, for the jump mechanic (was sendJump).
    JumpResult requestJump(int row, int col);

    // Advances the clock and lets the arbiter resolve anything that is due.
    void wait(long ms);

    Board &board() { return board_; }
    const Board &board() const { return board_; }
    bool gameOver() const { return gameOver_; }

private:
    Board board_;
    pieceRules::PieceRulesRegistry rules_;
    bool gameOver_ = false;
    long elapsedMs_ = 0;
    RealTimeArbiter arbiter_;
};

