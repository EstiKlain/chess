#include "GameEngine.hpp"

#include <iostream>
#include <sstream>

#include "GameOverRule.hpp"
#include "MoveLegality.hpp"
#include "Movement.hpp"
#include "Moves.hpp"
#include "RuleEngine.hpp"
#include "config.hpp"
#include "model/Board.hpp"

MoveResult GameEngine::requestMove(const MoveRequest &request)
{
    if (gameOver_)
        return {false, "game_over"};

    const int fromRow = request.from.row;
    const int fromCol = request.from.col;
    const int toRow = request.to.row;
    const int toCol = request.to.col;

    if (fromRow < 0 || fromCol < 0 || fromRow >= board_.rows() || fromCol >= board_.cols())
        return {false, "outside_board"};

    if (arbiter_.hasActiveMotion())
        return {false, "motion_in_progress"};

    if (arbiter_.hasActiveJumpAt(fromRow, fromCol))
        return {false, "jump_in_progress"};

    const Piece *selected = board_.pieceAt(Position{fromRow, fromCol});

    PieceMove m;
    m.fromRow = fromRow;
    m.fromCol = fromCol;
    m.toRow = toRow;
    m.toCol = toCol;
    m.startMs = elapsedMs_;
    m.pieceId = selected ? selected->id : -1;

    const char piece = selected ? selected->kind : '\0';
    const double speed = config::statsFor(piece).speedCellsPerSec;
    const double dist = cellDistance(fromRow, fromCol, toRow, toCol);
    m.durationMs = (speed > 0.0) ? (long)(dist / speed * 1000.0) : 0;

    const MoveLegality legality = isMoveLegal(board_, m, piece, rules_);
    if (!legality.isValid)
        return {false, legality.reason};

    arbiter_.startMotion(board_, m);
    return {true, legality.reason};
}

JumpResult GameEngine::requestJump(int row, int col)
{
    if (gameOver_)
        return {false, "game_over"};

    if (row < 0 || col < 0 || row >= board_.rows() || col >= board_.cols())
        return {false, "outside_board"};

    const Piece *selected = board_.pieceAt(Position{row, col});
    if (!selected)
        return {false, "empty_source"};

    if (arbiter_.isPieceInFlight(row, col))
        return {false, "motion_in_progress"};

    if (arbiter_.hasActiveJumpAt(row, col))
        return {false, "jump_in_progress"};

    JumpMove j;
    j.row = row;
    j.col = col;
    j.startMs = elapsedMs_;
    j.durationMs = config::JUMP_DURATION_MS;
    j.pieceId = selected->id;
    arbiter_.startJump(board_, j);
    return {true, "legal"};
}

void GameEngine::wait(long ms)
{
    if (gameOver_)
        return;

    elapsedMs_ += ms;
    std::vector<Piece> captured = arbiter_.resolveMoves(board_, elapsedMs_, rules_);
    if (isGameOver(captured))
        gameOver_ = true;
}
