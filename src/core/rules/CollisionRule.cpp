#include "rules/CollisionRule.hpp"

#include "rules/CaptureRule.hpp"

namespace
{
    // Cancel (stopAtIndex == 0, the collision is on this move's very first
    // path cell - nowhere earlier to stop) or Truncate (cut short to the
    // cell just before stopAtIndex, along this move's own path) for a
    // single mover. Shared by both the single-winner and the mutual-tie
    // cases below, so the "how do I stop early" computation exists once.
    CollisionDecision stopShortDecision(const PieceMove &move, size_t stopAtIndex)
    {
        if (stopAtIndex == 0)
            return CollisionDecision{CollisionDecision::Kind::Cancel, move.pieceId, {}, 0};

        CollisionDecision d;
        d.kind = CollisionDecision::Kind::Truncate;
        d.pieceId = move.pieceId;
        d.newDurationMs = (move.durationMs * static_cast<long>(stopAtIndex)) / static_cast<long>(move.path.size());
        d.newPath.assign(move.path.begin(), move.path.begin() + static_cast<long>(stopAtIndex));
        return d;
    }
}

std::vector<CollisionDecision> decideCollisionOutcome(const PieceMove &a, const Piece &moverA,
                                                        const PieceMove &b, const Piece &moverB,
                                                        const CellCollision &collision)
{
    const bool tie = (collision.arrivalA == collision.arrivalB);

    if (capturesOnArrival(moverA.color, moverB.color))
    {
        if (tie)
            return {
                CollisionDecision{CollisionDecision::Kind::Capture, moverA.id, {}, 0},
                CollisionDecision{CollisionDecision::Kind::Capture, moverB.id, {}, 0},
            };

        const int loserId = (collision.arrivalA < collision.arrivalB) ? moverA.id : moverB.id;
        return {CollisionDecision{CollisionDecision::Kind::Capture, loserId, {}, 0}};
    }

    if (tie)
        return {
            stopShortDecision(a, collision.indexA),
            stopShortDecision(b, collision.indexB),
        };

    const bool aArrivesLater = collision.arrivalA > collision.arrivalB;
    return {aArrivesLater ? stopShortDecision(a, collision.indexA)
                           : stopShortDecision(b, collision.indexB)};
}
