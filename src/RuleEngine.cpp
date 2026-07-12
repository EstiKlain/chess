#include "RuleEngine.hpp"
#include "Movement.hpp"

MoveLegality isMoveLegal(const Board &board, const PieceMove &move, char piece)
{
    if (move.toRow < 0 || move.toRow >= board.rows() || move.toCol < 0 || move.toCol >= board.cols())
    {
        return {false, "outside_board"};
    }
    return isLegalMove(board, move, piece);
}
