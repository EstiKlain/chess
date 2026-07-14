#include "rules/GameOverRule.hpp"

bool isGameOver(const std::vector<Piece> &capturedPieces)
{
    for (const auto &piece : capturedPieces)
    {
        if (piece.kind == 'K')
            return true;
    }
    return false;
}
