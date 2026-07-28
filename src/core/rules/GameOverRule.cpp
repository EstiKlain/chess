#include "rules/GameOverRule.hpp"

std::optional<char> winnerFromCaptured(const std::vector<Piece> &capturedPieces)
{
    for (const auto &piece : capturedPieces)
    {
        if (piece.kind == 'K')
            return piece.color == 'w' ? 'b' : 'w';
    }
    return std::nullopt;
}
