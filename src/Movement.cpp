#include "Movement.hpp"

#include <cmath>

#include "config.hpp"

int playerIndexOf(char color) {
    switch (color) {
        case 'w': return 0;
        case 'b': return 1;
        default:  return -1;
    }
}

double cellDistance(int r1, int c1, int r2, int c2) {
    double dr = r2 - r1, dc = c2 - c1;
    return std::sqrt(dr * dr + dc * dc);
}

bool isLegalMove(const Board& board, const PieceMove& move, char piece) {
    auto it = config::moveShapes.find(piece);
    if (it == config::moveShapes.end()) return true;   // no rule registered yet -> unrestricted for now

    const config::MoveRule& rule = it->second;
    char color = move.piece[0];

    const std::string &destination = board.grid[move.toRow][move.toCol];
    if (!isEmpty(destination) && colorOf(destination) == color) return false;

    bool isCapture = !isEmpty(destination);
    const config::MoveShapeFn& shape = (isCapture && rule.captureShape) ? rule.captureShape : rule.shape;

    int dRow = move.toRow - move.fromRow;
    int dCol = move.toCol - move.fromCol;
    if (!shape(dRow, dCol, color)) return false;

    if (rule.slides && !isPathClear(board, move.fromRow, move.fromCol, move.toRow, move.toCol))
        return false;

    return true;
}
