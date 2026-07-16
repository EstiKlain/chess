#include "realtime/RealTimeArbiter.hpp"

#include <algorithm>

#include "rules/Movement.hpp"
#include "rules/PieceRules.hpp"
#include "model/Board.hpp"
#include "config.hpp" 

void RealTimeArbiter::startRest(Board &board, int pieceId, long startMs, long durationMs, PieceState kind)
{
    Piece *p = board.pieceById(pieceId);
    if (!p)
        return;

    if (durationMs <= 0)
    {
        p->state = PieceState::Idle;
        return;
    }

    p->state = kind;
    activeRests_.push_back(RestWindow{pieceId, startMs, durationMs, kind});
}

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
            continue;

        bool reverseCaptured = false;
        for (const auto &j : activeJumps_)
        {
            if (j.row != m.toRow || j.col != m.toCol)
                continue;

            const Piece *jumper = board.pieceById(j.pieceId);
            if (!jumper || jumper->color == mover->color)
                continue;

            long jumpEndMs = j.startMs + j.durationMs;
            if (arrivalMs <= jumpEndMs)
            {
                mover->state = PieceState::Captured;
                captured.push_back(*mover);
                board.removePiece(mover->id);
                reverseCaptured = true;
            }
            break;
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
            // TODO
            // הכתרה של חייל למלכה לא אמורה להיות בזמן אמת !

            if (Piece *freshMover = board.pieceById(moverId))
            {
                if (freshMover->kind == 'P' && m.toRow == registry.pawnPromotionRow(freshMover->color, board.rows()))
                    freshMover->kind = 'Q';
                // CHANGED: was `freshMover->state = PieceState::Idle;`
                startRest(board, moverId, arrivalMs, config::statsFor(freshMover->kind).longRestMs, PieceState::RestingLong);
            }
        }
        else if (destination == nullptr)
        {
            board.movePiece(mover->id, destinationCell);
            if (mover->kind == 'P' && m.toRow == registry.pawnPromotionRow(mover->color, board.rows()))
                mover->kind = 'Q';
            startRest(board, mover->id, arrivalMs, config::statsFor(mover->kind).longRestMs, PieceState::RestingLong);
        }
        else
        {
            // Friendly piece blocks destination - move failed, no real
            // move happened, so no cooldown either. Stays Idle.
            mover->state = PieceState::Idle;
        }
    }

    activeMoves_ = stillMoving;

    std::vector<JumpMove> stillJumping;
    for (const auto &j : activeJumps_)
    {
        if (elapsedMs >= j.startMs + j.durationMs)
        {
            if (Piece *p = board.pieceById(j.pieceId))
                // CHANGED: was `p->state = PieceState::Idle;`
                startRest(board, p->id, j.startMs + j.durationMs, config::statsFor(p->kind).shortRestMs, PieceState::RestingShort);
            continue;
        }
        stillJumping.push_back(j);
    }
    activeJumps_ = stillJumping;

    std::vector<RestWindow> stillResting;
    for (const auto &r : activeRests_)
    {
        if (elapsedMs >= r.startMs + r.durationMs)
        {
            if (Piece *p = board.pieceById(r.pieceId))
                p->state = PieceState::Idle;
            continue;
        }
        stillResting.push_back(r);
    }
    activeRests_ = stillResting;

    return captured;
}

bool RealTimeArbiter::isPieceInFlight(int row, int col) const
{
    for (const PieceMove &move : activeMoves_)
        if (move.fromRow == row && move.fromCol == col)
            return true;
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
        p->state = PieceState::Jumping; // CHANGED: was Moving
}

std::optional<PieceMove> RealTimeArbiter::activeMoveForPiece(int pieceId) const
{
    for (const PieceMove &m : activeMoves_)
        if (m.pieceId == pieceId)
            return m;
    return std::nullopt;
}

std::optional<JumpMove> RealTimeArbiter::activeJumpForPiece(int pieceId) const // NEW
{
    for (const auto &j : activeJumps_)
        if (j.pieceId == pieceId)
            return j;
    return std::nullopt;
}

bool RealTimeArbiter::isPieceResting(int pieceId) const // NEW
{
    for (const auto &r : activeRests_)
        if (r.pieceId == pieceId)
            return true;
    return false;
}

std::optional<RestWindow> RealTimeArbiter::activeRestForPiece(int pieceId) const // NEW
{
    for (const auto &r : activeRests_)
        if (r.pieceId == pieceId)
            return r;
    return std::nullopt;
}