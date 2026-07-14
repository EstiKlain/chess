#pragma once

namespace config
{

    constexpr int CELL_SIZE = 100;

    constexpr int NUM_PLAYERS = 2;

    constexpr long JUMP_DURATION_MS = 1000;
    struct PieceStats
    {
        double speedCellsPerSec;
        long restMs; // cooldown between consecutive moves per piece
    };

    inline PieceStats statsFor(char piece)
    {
        switch (piece)
        {
        case 'Q':
            return {4.0, 0};
        case 'R':
            return {1.0, 0};
        case 'B':
            return {3.0, 0};
        case 'N':
            return {3.5, 0};
        case 'K':
            return {3.0, 0};
        case 'P':
            return {2.0, 0};
        default:
            return {0.0, 0};
        }
    }
}
