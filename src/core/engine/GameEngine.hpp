#pragma once

#include <optional>
#include <string>
#include <vector>

#include "model/Board.hpp"
#include "engine/MoveRequest.hpp"
#include "engine/GameSnapshot.hpp"
#include "realtime/RealTimeArbiter.hpp"
#include "rules/PieceRules.hpp"
#include "rules/GameOverRule.hpp"

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

    /// Ends the game immediately in the opponent's favor. No-op (returns
    /// false) if the game is already over - a same-tick king-capture (via
    /// wait()) must win any race against a disconnect-timeout resign, never
    /// get overwritten by it.
    bool resign(char color);

    // The single read-only gate OUT of the game, mirroring requestMove/
    // requestJump as the single gate IN. Returns a fresh value-copy every
    // call (Stage/Iteration D) - see GameSnapshot.hpp for why.
    GameSnapshot snapshot() const;

    Board &board() { return board_; }
    const Board &board() const { return board_; }
    bool gameOver() const { return gameOver_; }

private:
    Board board_;
    pieceRules::PieceRulesRegistry rules_;
    bool gameOver_ = false;
    std::optional<char> winner_;
    std::optional<GameOverReason> gameOverReason_;
    long elapsedMs_ = 0;
    RealTimeArbiter arbiter_;
};

