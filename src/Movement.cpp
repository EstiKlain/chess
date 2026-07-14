#include "Movement.hpp"

#include <cmath>

#include "PieceRules.hpp"

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

MoveLegality checkPieceShape(const Board &board, const PieceMove &move, char piece, const pieceRules::PieceRulesRegistry &registry) {
    const pieceRules::MoveRule *rule = registry.find(piece);
    if (!rule) return {true, "legal"};

    const Piece *mover = board.pieceById(move.pieceId);
    char color = mover ? mover->color : '\0';

    const Piece *destination = board.pieceAt(Position{move.toRow, move.toCol});
    if (destination != nullptr && destination->color == color) return {false, "friendly_destination"};

    bool isCapture = destination != nullptr;
    const pieceRules::MoveShapeFn &shape = (isCapture && rule->captureShape) ? rule->captureShape : rule->shape;

    int dRow = move.toRow - move.fromRow;
    int dCol = move.toCol - move.fromCol;
    if (!shape(dRow, dCol, color)) return {false, "illegal_piece_move"};

    if (rule->slides && !pieceRules::isPathClear(board, move.fromRow, move.fromCol, move.toRow, move.toCol))
        return {false, "blocked_path"};

    if (rule->contextGate && !rule->contextGate(board, move.fromRow, move.fromCol, move.toRow, move.toCol, color))
        return {false, "pawn_double_step_blocked"};

    return {true, "legal"};
}
