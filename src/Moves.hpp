#pragma once

#include <string>

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
    std::string piece;
};

// A piece currently airborne (jump in progress).
// Owned privately by RealTimeArbiter.
struct JumpMove
{
    int row, col;
    long startMs;
    long durationMs;
    std::string piece;
};
