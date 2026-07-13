#include "RuleEngine.hpp"
#include "Movement.hpp"

MoveLegality isMoveLegal(const Board &board, const PieceMove &move, char piece, const pieceRules::PieceRulesRegistry &registry)
{
    if (isEmpty(board.grid[move.fromRow][move.fromCol]))
    {
        return {false, "empty_source"};
    }
    if (move.toRow < 0 || move.toRow >= board.rows() || move.toCol < 0 || move.toCol >= board.cols())
    {
        return {false, "outside_board"};
    }
    return checkPieceShape(board, move, piece, registry);
}
