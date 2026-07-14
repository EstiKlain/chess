#include "realtime/RealTimeArbiter.hpp"

#include <algorithm>

#include "rules/Movement.hpp"
#include "rules/PieceRules.hpp"
#include "model/Board.hpp"

std::vector<Piece> RealTimeArbiter::resolveMoves(Board &board, long elapsedMs, const pieceRules::PieceRulesRegistry &registry)
{
    std::vector<Piece> captured;
    std::vector<size_t> due;
    std::vector<PieceMove> stillMoving;
    for (size_t i = 0; i < activeMoves_.size(); ++i)
    {
        const PieceMove &m = activeMoves_[i];
        if (elapsedMs >= m.startMs + m.durationMs)
            due.push_back(i);
        else
            stillMoving.push_back(m);
    }

    std::sort(due.begin(), due.end(), [&](size_t a, size_t b)
              {
        long ta = activeMoves_[a].startMs + activeMoves_[a].durationMs;
        long tb = activeMoves_[b].startMs + activeMoves_[b].durationMs;
        if (ta != tb) return ta < tb;
        return a < b; });

    for (size_t idx : due)
    {
        const PieceMove &m = activeMoves_[idx];
        long arrivalMs = m.startMs + m.durationMs;

        Piece *mover = board.pieceById(m.pieceId);
        if (!mover)
            continue; // mover no longer exists (e.g. captured earlier this batch) - nothing to resolve

        bool reverseCaptured = false;
        for (const auto &j : activeJumps_)
        {
            if (j.row != m.toRow || j.col != m.toCol)
                continue;

            const Piece *jumper = board.pieceById(j.pieceId);
            if (!jumper || jumper->color == mover->color)
                continue; // only an enemy jump defends

            long jumpEndMs = j.startMs + j.durationMs;
            if (arrivalMs <= jumpEndMs)
            {
                // the jumper wins: the arriving piece is captured,
                // the jumper stays exactly where it was (rule 2)
                mover->state = PieceState::Captured;
                captured.push_back(*mover);
                board.removePiece(mover->id);
                reverseCaptured = true;
            }
            break; // at most one jump can occupy a given cell
        }
        if (reverseCaptured)
            continue;

        const Position destinationCell{m.toRow, m.toCol};
        Piece *destination = board.pieceAt(destinationCell);

        if (destination != nullptr && destination->color != mover->color)
        {
            destination->state = PieceState::Captured;
            captured.push_back(*destination);
            const int moverId = mover->id;
            board.removePiece(destination->id);

            board.movePiece(mover->id, destinationCell);
            if (Piece *freshMover = board.pieceById(moverId))
            {
                // TODO
                // הכתרה של חייל למלכה לא אמורה להיות בזמן אמת !
                freshMover->state = PieceState::Idle;
                if (freshMover->kind == 'P' && m.toRow == registry.pawnPromotionRow(freshMover->color, board.rows()))
                    freshMover->kind = 'Q'; // שימוש ב-freshMover המעודכן מהלוח!
            }
        }
        else if (destination == nullptr)
        {
            board.movePiece(mover->id, destinationCell);
            mover->state = PieceState::Idle;
            if (mover->kind == 'P' && m.toRow == registry.pawnPromotionRow(mover->color, board.rows()))
                mover->kind = 'Q';
        }
        // else: friendly piece blocks destination -> move fails, piece stays
        // at origin and simply returns to Idle (it never actually left).
        else
        {
            mover->state = PieceState::Idle;
        }
    }

    activeMoves_ = stillMoving;

    std::vector<JumpMove> stillJumping;
    for (const auto &j : activeJumps_)
    {
        if (elapsedMs >= j.startMs + j.durationMs)
        {
            // landed: if the piece is still there (wasn't captured mid-air
            // by a defended arrival above, which already erased it), it
            // simply returns to Idle - it never moved.
            if (Piece *p = board.pieceById(j.pieceId))
                p->state = PieceState::Idle;
            continue;
        }
        stillJumping.push_back(j);
    }
    activeJumps_ = stillJumping;
    return captured;
}

bool RealTimeArbiter::isPieceInFlight(int row, int col) const
{
    for (const PieceMove &move : activeMoves_)
    {
        if (move.fromRow == row && move.fromCol == col)
        {
            return true;
        }
    }
    return false;
}

bool RealTimeArbiter::hasActiveMotion() const
{
    return !activeMoves_.empty();
}

void RealTimeArbiter::startMotion(Board &board, const PieceMove &move)
{
    activeMoves_.push_back(move);
    if (Piece *p = board.pieceById(move.pieceId))
        p->state = PieceState::Moving;
}

bool RealTimeArbiter::hasActiveJumpAt(int row, int col) const
{
    for (const auto &j : activeJumps_)
        if (j.row == row && j.col == col)
            return true;
    return false;
}

void RealTimeArbiter::startJump(Board &board, const JumpMove &jump)
{
    activeJumps_.push_back(jump);
    if (Piece *p = board.pieceById(jump.pieceId))
        p->state = PieceState::Moving;
}
