#pragma once

namespace config
{

    constexpr int CELL_SIZE = 100;

    constexpr int NUM_PLAYERS = 2;

    constexpr long JUMP_DURATION_MS = 1000;
    struct PieceStats
    {
        double speedCellsPerSec;
        long shortRestMs;
        long longRestMs;
    };

    inline PieceStats statsFor(char piece)
    {
        switch (piece)
        {
        case 'Q':
            return {4.0, 300, 900};
        case 'R':
            return {1.0, 250, 700};
        case 'B':
            return {3.0, 250, 700};
        case 'N':
            return {3.5, 300, 800};
        case 'K':
            return {3.0, 400, 1200};
        case 'P':
            return {2.0, 200, 500};
        default:
            return {0.0, 0, 0};
        }
    }
}
