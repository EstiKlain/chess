#pragma once

#include <vector>

#include "model/Piece.hpp"
#include "model/Position.hpp"
// A move currently "in flight" between two cells.
// Owned privately by RealTimeArbiter; also used as the shape-check
// input for RuleEngine/Movement (checkPieceShape), which is why it
// lives in its own header instead of inside RealTimeArbiter.hpp -
// that avoids Movement.hpp/RuleEngine.hpp having to depend on the
// whole RealTimeArbiter class just to see this struct.
struct PieceMove
{
    int fromRow, fromCol;
    int toRow, toCol;
    long startMs;
    long durationMs;
    int pieceId;

    // The cells this move traverses (destination included, source
    // excluded), pre-computed once by GameEngine from the mover's
    // MoveRule::pathCells at request time - empty for moves with no
    // intermediate cell (a knight's hop). RealTimeArbiter reads this
    // directly for mid-flight collision detection; it never consults
    // PieceRulesRegistry itself (see RealTimeArbiter::resolveMidFlightCollisions).
    std::vector<Position> path;
};

// A piece currently airborne (jump in progress).
// Owned privately by RealTimeArbiter.
struct JumpMove
{
    int row, col;
    long startMs;
    long durationMs;
    int pieceId;
};

struct RestWindow
{
    int pieceId;
    long startMs;
    long durationMs;
    PieceState kind; // RestingShort or RestingLong
};
