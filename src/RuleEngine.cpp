#include "RuleEngine.hpp"
#include "Movement.hpp"

bool isMoveLegal(const Board &board, const PieceMove &move, char piece)
{
    return isLegalMove(board, move, piece);
}
