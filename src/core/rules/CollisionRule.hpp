#pragma once

#include <vector>

#include "model/Piece.hpp"
#include "model/Position.hpp"
#include "realtime/Moves.hpp"

// A confirmed geometric+timing overlap between two in-flight moves' own
// paths: which index of each move's own path the shared cell sits at, and
// each move's computed arrival there. Carries no notion of who "wins" -
// that is decideCollisionOutcome's job, not this one's. Produced by
// RealTimeArbiter::findPathCollision (pure real-time clock arithmetic,
// stays there - this struct is just the shared shape both sides agree on).
struct CellCollision
{
    size_t indexA;
    size_t indexB;
    long arrivalA;
    long arrivalB;
};

// What to do about a confirmed collision - filled in by
// decideCollisionOutcome, acted on by RealTimeArbiter.
struct CollisionDecision
{
    enum class Kind
    {
        Capture,  // different colors - pieceId is the losing mover
        Cancel,   // same color, collision on pieceId's very first path cell
        Truncate, // same color - pieceId's move is cut short to newPath/newDurationMs
    };

    Kind kind;
    int pieceId;
    std::vector<Position> newPath;  // Truncate only
    long newDurationMs = 0;         // Truncate only
};

// Pure game-rule: given a confirmed collision (and the two movers'
// colors), decide the outcome - one decision per affected piece.
//
// Different colors: capturesOnArrival decides who eats whom - normally
// the later arrival eats the earlier one (1 decision). On an EXACT tie
// (arrivalA == arrivalB - not a rare case, since GameEngine quantizes
// startMs to the caller's own tick, so two moves requested in the same
// tick share it exactly) neither is "later" - this is a symmetric,
// head-on mutual capture: both movers are captured (2 decisions).
//
// Same color: the later mover is stopped one cell short of the shared
// cell, along its OWN path, never the other mover's (1 decision). On an
// exact tie, neither is "later" - both stop one cell short, each along
// its own path (2 decisions).
//
// RealTimeArbiter calls this but never decides the outcome itself - same
// pattern as GameOverRule.hpp's winnerFromCaptured.
std::vector<CollisionDecision> decideCollisionOutcome(const PieceMove &a, const Piece &moverA,
                                                        const PieceMove &b, const Piece &moverB,
                                                        const CellCollision &collision);
