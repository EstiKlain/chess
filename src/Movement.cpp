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

MoveLegality checkPieceShape(const Board& board, const PieceMove& move, char piece) {
    auto it = config::moveShapes.find(piece);
    if (it == config::moveShapes.end()) return {true, "legal"};  

    const config::MoveRule& rule = it->second;
    char color = move.piece[0];

    const std::string &destination = board.grid[move.toRow][move.toCol];
    if (!isEmpty(destination) && colorOf(destination) == color) return {false, "friendly_destination"};

    bool isCapture = !isEmpty(destination);
    const config::MoveShapeFn& shape = (isCapture && rule.captureShape) ? rule.captureShape : rule.shape;

    int dRow = move.toRow - move.fromRow;
    int dCol = move.toCol - move.fromCol;
    if (!shape(dRow, dCol, color)) return {false, "illegal_piece_move"};

    if (rule.slides && !isPathClear(board, move.fromRow, move.fromCol, move.toRow, move.toCol))
        return {false, "blocked_path"};

    if (rule.contextGate && !rule.contextGate(board, move.fromRow, move.fromCol, move.toRow, move.toCol, color))
        return {false, "pawn_double_step_blocked"};

    return {true, "legal"};
}

int sign(int v) { return (v > 0) - (v < 0); }
 
bool isPathClear(const Board& board, int fromRow, int fromCol, int toRow, int toCol) {
    int stepRow = sign(toRow - fromRow);
    int stepCol = sign(toCol - fromCol);
 
    int r = fromRow + stepRow, c = fromCol + stepCol;
    while (r != toRow || c != toCol) {
        if (!isEmpty(board.grid[r][c])) return false;
        r += stepRow;
        c += stepCol;
    }
    return true;
}
 
