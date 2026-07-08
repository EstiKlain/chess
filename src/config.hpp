#pragma once

#include <cmath>
#include <functional>
#include <map>

namespace config {

    constexpr int CELL_SIZE = 100;

    constexpr int NUM_PLAYERS = 2;

    struct PieceStats {
        double speedCellsPerSec;
        long   restMs;
    };

    inline PieceStats statsFor(char piece) {
        switch (piece) {
            case 'Q': return {4.0, 0};
            case 'R': return {3.0, 0};
            case 'B': return {3.0, 0};
            case 'N': return {3.5, 0};
            case 'K': return {3.0, 0};
            case 'P': return {2.0, 0};
            default:  return {0.0, 0};
        }
    }

    using MoveShapeFn = std::function<bool(int dRow, int dCol, char color)>;

    inline bool kingShape(int dRow, int dCol, char /*color*/) {
        return (dRow != 0 || dCol != 0) && std::abs(dRow) <= 1 && std::abs(dCol) <= 1;
    }
    inline bool rookShape(int dRow, int dCol, char /*color*/) {
        return (dRow == 0) != (dCol == 0);
    }
    inline bool bishopShape(int dRow, int dCol, char /*color*/) {
        return dRow != 0 && std::abs(dRow) == std::abs(dCol);
    }
    inline bool queenShape(int dRow, int dCol, char /*color*/) {
        return rookShape(dRow, dCol, ' ') || bishopShape(dRow, dCol, ' ');
    }
    inline bool knightShape(int dRow, int dCol, char /*color*/) {
        int r = std::abs(dRow), c = std::abs(dCol);
        return (r == 1 && c == 2) || (r == 2 && c == 1);
    }
    inline int pawnForwardDir(char color) {
        return (color == 'w') ? -1 : 1;
    }
    inline bool pawnShape(int dRow, int dCol, char color) {
        return dCol == 0 && dRow == pawnForwardDir(color);
    }
    inline bool pawnCaptureShape(int dRow, int dCol, char color) {
        return std::abs(dCol) == 1 && dRow == pawnForwardDir(color);
    }

    struct MoveRule {
        MoveShapeFn shape;
        bool        slides;
        MoveShapeFn captureShape = nullptr;
    };

    inline std::map<char, MoveRule> moveShapes = {
        {'K', {kingShape,   false}},
        {'Q', {queenShape,  true }},
        {'R', {rookShape,   true }},
        {'B', {bishopShape, true }},
        {'N', {knightShape, false}},
        {'P', {pawnShape,   false, pawnCaptureShape}},
    };
}
