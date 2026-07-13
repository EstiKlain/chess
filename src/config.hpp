#pragma once

#include <cmath>
#include <functional>
#include <map>

#include "Board.hpp"

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

    using MoveShapeFn = std::function<bool(int dRow, int dCol, char color)>;

    inline bool kingShape(int dRow, int dCol, char /*color*/)
    {
        return (dRow != 0 || dCol != 0) && std::abs(dRow) <= 1 && std::abs(dCol) <= 1;
    }
    inline bool rookShape(int dRow, int dCol, char /*color*/)
    {
        return (dRow == 0) != (dCol == 0);
    }
    inline bool bishopShape(int dRow, int dCol, char /*color*/)
    {
        return dRow != 0 && std::abs(dRow) == std::abs(dCol);
    }
    inline bool queenShape(int dRow, int dCol, char /*color*/)
    {
        return rookShape(dRow, dCol, ' ') || bishopShape(dRow, dCol, ' ');
    }
    inline bool knightShape(int dRow, int dCol, char /*color*/)
    {
        int r = std::abs(dRow), c = std::abs(dCol);
        return (r == 1 && c == 2) || (r == 2 && c == 1);
    }
    inline int pawnForwardDir(char color)
    {
        return (color == 'w') ? -1 : 1;
    }
    inline int pawnStartRow(char color, int totalRows)
    {
        return (color == 'w') ? totalRows - 2 : 1;
    }
    inline int pawnPromotionRow(char color, int totalRows)
    {
        return (color == 'w') ? 0 : totalRows - 1;
    }
    inline bool pawnShape(int dRow, int dCol, char color)
    {
        int fwd = pawnForwardDir(color);
        return dCol == 0 && (dRow == fwd || dRow == 2 * fwd);
    }
    inline bool pawnCaptureShape(int dRow, int dCol, char color)
    {
        return std::abs(dCol) == 1 && dRow == pawnForwardDir(color);
    }

    struct MoveRule
    {
        MoveShapeFn shape;
        bool slides;
        MoveShapeFn captureShape = nullptr;
        std::function<bool(const Board &, int fromRow, int fromCol, int toRow, int toCol, char color)> contextGate = nullptr;
    };

    inline bool pawnContextGate(const Board &board, int fromRow, int fromCol, int toRow, int toCol, char color)
    {
        int dRow = toRow - fromRow;
        int dCol = toCol - fromCol;
        int doubleStep = 2 * pawnForwardDir(color);
        if (dCol != 0 || dRow != doubleStep)
            return true;
        if (fromRow != pawnStartRow(color, board.rows()))
            return false;
        return isPathClear(board, fromRow, fromCol, toRow, toCol);
    }

    inline std::map<char, MoveRule> moveShapes = {
        {'K', {kingShape, false}},
        {'Q', {queenShape, true}},
        {'R', {rookShape, true}},
        {'B', {bishopShape, true}},
        {'N', {knightShape, false}},
        {'P', {pawnShape, false, pawnCaptureShape, pawnContextGate}},
    };
}
