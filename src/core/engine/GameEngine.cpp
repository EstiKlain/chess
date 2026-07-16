#include "engine/GameEngine.hpp"

#include <iostream>
#include <sstream>

#include "rules/GameOverRule.hpp"
#include "rules/MoveLegality.hpp"
#include "rules/Movement.hpp"
#include "realtime/Moves.hpp"
#include "rules/RuleEngine.hpp"
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

    if (selected && arbiter_.isPieceResting(selected->id))
        return {false, "resting"};

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

    if (arbiter_.isPieceResting(selected->id)) 
        return {false, "resting"};

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

GameSnapshot GameEngine::snapshot() const
{
    GameSnapshot s;
    s.rows = board_.rows();
    s.cols = board_.cols();
    s.gameOver = gameOver_;
    s.nowMs = elapsedMs_;

    s.pieces.reserve(board_.pieces().size());
    for (const Piece &p : board_.pieces())
    {
        PieceSnapshot ps{p.id, p.color, p.kind, p.cell.row, p.cell.col};
        ps.state = p.state; 

        if (const std::optional<PieceMove> move = arbiter_.activeMoveForPiece(p.id))
        {
            ps.motion = MotionSnapshot{move->fromRow, move->fromCol,
                                        move->toRow, move->toCol,
                                        move->startMs, move->durationMs};
            ps.stateStartMs = move->startMs;
            ps.stateDurationMs = move->durationMs;
        }
        else if (const std::optional<JumpMove> jump = arbiter_.activeJumpForPiece(p.id)) 
        {
            ps.stateStartMs = jump->startMs;
            ps.stateDurationMs = jump->durationMs;
        }
        else if (const std::optional<RestWindow> rest = arbiter_.activeRestForPiece(p.id)) 
        {
            ps.stateStartMs = rest->startMs;
            ps.stateDurationMs = rest->durationMs;
        }

        s.pieces.push_back(ps);
    }

    return s;
}