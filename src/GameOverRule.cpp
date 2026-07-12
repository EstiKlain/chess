#include "GameOverRule.hpp"
#include "Board.hpp"

bool isGameOver(const std::vector<std::string> &capturedTokens)
{
    for (const auto &tok : capturedTokens)
    {
        if (pieceOf(tok) == 'K')
            return true;
    }
    return false;
}
