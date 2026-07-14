#include "rules/RuleEngine.hpp"
#include "rules/Movement.hpp"

MoveLegality isMoveLegal(const Board &board, const PieceMove &move, char piece, const pieceRules::PieceRulesRegistry &registry)
{
    if (board.pieceAt(Position{move.fromRow, move.fromCol}) == nullptr)
    {
        return {false, "empty_source"};
    }
    if (move.toRow < 0 || move.toRow >= board.rows() || move.toCol < 0 || move.toCol >= board.cols())
    {
        return {false, "outside_board"};
    }
    return checkPieceShape(board, move, piece, registry);
}
