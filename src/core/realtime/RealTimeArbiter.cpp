#include "realtime/RealTimeArbiter.hpp"

#include <algorithm>
#include <cstdlib>
#include <optional>

#include "rules/Movement.hpp"
#include "rules/PieceRules.hpp"
#include "rules/CaptureRule.hpp"
#include "rules/CollisionRule.hpp"
#include "rules/PromotionRule.hpp"
#include "model/Board.hpp"
#include "config.hpp"

namespace
{
    // How close two independently-computed cell-arrival timestamps need to
    // be to count as "the same instant" for collision purposes. Not tied to
    // any particular caller's tick rate (callers may call resolveMoves at
    // any cadence) - it's a domain-level slack for two moves that a player
    // would reasonably perceive as arriving together.
    constexpr long kMidFlightCollisionToleranceMs = 50;

    // The ms at which a move reaches the (pathIndex)'th cell of its own
    // path (0-based, path already excludes the source cell) - a straight
    // proportional split of the move's own total duration, the same way
    // durationMs itself is derived from distance/speed.
    long cellArrivalMs(const PieceMove &move, size_t pathIndex, size_t pathLength)
    {
        return move.startMs + (move.durationMs * static_cast<long>(pathIndex + 1)) / static_cast<long>(pathLength);
    }

    // Pure geometry + timing: do these two paths share a cell, arrived at
    // within tolerance of each other, at an instant elapsedMs has already
    // reached? This is real-time clock arithmetic over already-known
    // PieceMove data, not a game-rule decision -
    // CellCollision/CollisionDecision/decideCollisionOutcome
    // (rules/CollisionRule.hpp) are what decide what a found collision
    // actually means.
    //
    // Stops at the first qualifying cell found, in `a`'s own travel order
    // (ascending ia). This is provably always the temporally-earliest
    // qualifying instant, not just "whichever the loop happened to hit
    // first": arrivalA strictly increases with ia, so the loop's first hit
    // always has the smallest arrivalA among all candidates. Two straight
    // lines meeting head-on (opposite directions) can have at most one
    // candidate cell in practice - the combined closing speed makes the
    // time-difference change by far more than kMidFlightCollisionToleranceMs
    // per cell for any realistic piece speed (config::statsFor), so a
    // second cell can't also qualify. Two lines running the same direction
    // can have several candidates only when the two movers are at
    // (near-)identical speed, in which case the time offset is constant
    // across the whole overlap - so the smallest ia is also the smallest
    // arrivalB, not just the smallest arrivalA. Either way, first-found
    // equals earliest-in-time. This stops holding only if
    // kMidFlightCollisionToleranceMs is ever made large relative to a
    // typical per-cell duration - revisit this analysis if it is.
    std::optional<CellCollision> findPathCollision(const PieceMove &a, const PieceMove &b, long elapsedMs)
    {
        for (size_t ia = 0; ia < a.path.size(); ++ia)
        {
            for (size_t ib = 0; ib < b.path.size(); ++ib)
            {
                if (!(a.path[ia] == b.path[ib]))
                    continue;

                const long arrivalA = cellArrivalMs(a, ia, a.path.size());
                const long arrivalB = cellArrivalMs(b, ib, b.path.size());
                if (std::abs(arrivalA - arrivalB) > kMidFlightCollisionToleranceMs)
                    continue;
                if (elapsedMs < std::min(arrivalA, arrivalB))
                    continue; // scheduled, but that instant hasn't happened yet

                return CellCollision{ia, ib, arrivalA, arrivalB};
            }
        }
        return std::nullopt;
    }
}

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

std::vector<Piece> RealTimeArbiter::resolveMidFlightCollisions(Board &board, long elapsedMs)
{
    struct Truncation
    {
        int pieceId;
        std::vector<Position> newPath; // truncated prefix of the mover's own path; newPath.back() is the new destination
        long newDurationMs;
    };

    std::vector<int> toCapture;
    std::vector<int> toCancel;
    std::vector<Truncation> toTruncate;
    std::vector<int> resolved; // piece ids already decided this call - never redecide

    auto isResolved = [&](int pieceId)
    { return std::find(resolved.begin(), resolved.end(), pieceId) != resolved.end(); };

    for (size_t i = 0; i < activeMoves_.size(); ++i)
    {
        for (size_t j = i + 1; j < activeMoves_.size(); ++j)
        {
            const PieceMove &a = activeMoves_[i];
            const PieceMove &b = activeMoves_[j];
            if (isResolved(a.pieceId) || isResolved(b.pieceId))
                continue;

            const Piece *moverA = board.pieceById(a.pieceId);
            const Piece *moverB = board.pieceById(b.pieceId);
            if (!moverA || !moverB)
                continue;

            // a.path/b.path were pre-computed by GameEngine at request time
            // from the mover's own MoveRule::pathCells - this function never
            // asks "what piece is this" or consults PieceRulesRegistry.
            if (a.path.empty() || b.path.empty())
                continue;

            const std::optional<CellCollision> collision = findPathCollision(a, b, elapsedMs);
            if (!collision)
                continue;

            // Usually one decision (a clear winner); exactly two on an
            // exact-arrival tie (mutual capture, or both stopped short).
            for (const CollisionDecision &decision : decideCollisionOutcome(a, *moverA, b, *moverB, *collision))
            {
                resolved.push_back(decision.pieceId);
                switch (decision.kind)
                {
                case CollisionDecision::Kind::Capture:
                    toCapture.push_back(decision.pieceId);
                    break;
                case CollisionDecision::Kind::Cancel:
                    toCancel.push_back(decision.pieceId);
                    break;
                case CollisionDecision::Kind::Truncate:
                    toTruncate.push_back({decision.pieceId, decision.newPath, decision.newDurationMs});
                    break;
                }
            }
        }
    }

    std::vector<Piece> captured;
    for (int pieceId : toCapture)
    {
        if (Piece *p = board.pieceById(pieceId))
        {
            p->state = PieceState::Captured;
            captured.push_back(*p);
            board.removePiece(pieceId);
        }
        activeMoves_.erase(std::remove_if(activeMoves_.begin(), activeMoves_.end(),
                                           [&](const PieceMove &m)
                                           { return m.pieceId == pieceId; }),
                            activeMoves_.end());
    }

    for (const Truncation &t : toTruncate)
    {
        auto it = std::find_if(activeMoves_.begin(), activeMoves_.end(),
                                [&](const PieceMove &m)
                                { return m.pieceId == t.pieceId; });
        if (it != activeMoves_.end())
            truncateMotion(*it, t.newPath, t.newDurationMs);
    }

    for (int pieceId : toCancel)
        cancelMotion(board, pieceId);

    return captured;
}

void RealTimeArbiter::truncateMotion(PieceMove &move, std::vector<Position> newPath, long newDurationMs)
{
    const Position &newDestination = newPath.back();
    move.toRow = newDestination.row;
    move.toCol = newDestination.col;
    move.durationMs = newDurationMs;
    move.path = std::move(newPath);
}

void RealTimeArbiter::cancelMotion(Board &board, int pieceId)
{
    if (Piece *p = board.pieceById(pieceId))
        p->state = PieceState::Idle;

    activeMoves_.erase(std::remove_if(activeMoves_.begin(), activeMoves_.end(),
                                       [&](const PieceMove &m)
                                       { return m.pieceId == pieceId; }),
                        activeMoves_.end());
}

std::vector<Piece> RealTimeArbiter::resolveMoves(Board &board, long elapsedMs, const pieceRules::PieceRulesRegistry &registry)
{
    std::vector<Piece> captured = resolveMidFlightCollisions(board, elapsedMs);
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
            if (!jumper || !capturesOnArrival(mover->color, jumper->color))
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

        if (destination != nullptr && capturesOnArrival(mover->color, destination->color))
        {
            destination->state = PieceState::Captured;
            captured.push_back(*destination);
            const int moverId = mover->id;
            board.removePiece(destination->id);

            board.movePiece(mover->id, destinationCell);

            if (Piece *freshMover = board.pieceById(moverId))
            {
                freshMover->kind = promotedKind(freshMover->kind, freshMover->color, m.toRow, board.rows(), registry);
                // CHANGED: was `freshMover->state = PieceState::Idle;`
                startRest(board, moverId, arrivalMs, config::statsFor(freshMover->kind).longRestMs, PieceState::RestingLong);
            }
        }
        else if (destination == nullptr)
        {
            board.movePiece(mover->id, destinationCell);
            mover->kind = promotedKind(mover->kind, mover->color, m.toRow, board.rows(), registry);
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
