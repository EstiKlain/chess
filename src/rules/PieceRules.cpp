#include "rules/PieceRules.hpp"
#include <cmath>

namespace pieceRules
{
    bool kingShape(int dRow, int dCol, char /*color*/)
    {
        return (dRow != 0 || dCol != 0) && std::abs(dRow) <= 1 && std::abs(dCol) <= 1;
    }

    bool rookShape(int dRow, int dCol, char /*color*/)
    {
        return (dRow == 0) != (dCol == 0);
    }

    bool bishopShape(int dRow, int dCol, char /*color*/)
    {
        return dRow != 0 && std::abs(dRow) == std::abs(dCol);
    }

    bool queenShape(int dRow, int dCol, char /*color*/)
    {
        return rookShape(dRow, dCol, ' ') || bishopShape(dRow, dCol, ' ');
    }

    bool knightShape(int dRow, int dCol, char /*color*/)
    {
        int r = std::abs(dRow), c = std::abs(dCol);
        return (r == 1 && c == 2) || (r == 2 && c == 1);
    }

    int pawnForwardDir(char color)
    {
        return (color == 'w') ? -1 : 1;
    }

    int pawnStartRow(char color, int totalRows)
    {
        return (color == 'w') ? totalRows - 2 : 1;
    }

    bool pawnShape(int dRow, int dCol, char color)
    {
        int fwd = pawnForwardDir(color);
        return dCol == 0 && (dRow == fwd || dRow == 2 * fwd);
    }

    bool pawnCaptureShape(int dRow, int dCol, char color)
    {
        return std::abs(dCol) == 1 && dRow == pawnForwardDir(color);
    }

    int sign(int v)
    {
        return (v > 0) - (v < 0);
    }

    bool isPathClear(const Board &board, int fromRow, int fromCol, int toRow, int toCol)
    {
        int stepRow = sign(toRow - fromRow);
        int stepCol = sign(toCol - fromCol);

        int r = fromRow + stepRow, c = fromCol + stepCol;
        while (r != toRow || c != toCol)
        {
            if (board.pieceAt(Position{r, c}) != nullptr)
                return false;
            r += stepRow;
            c += stepCol;
        }
        return true;
    }

    bool pawnContextGate(const Board &board, int fromRow, int fromCol, int toRow, int toCol, char color)
    {
        int dRow = toRow - fromRow;
        int dCol = toCol - fromCol;
        int doubleStep = 2 * pawnForwardDir(color);
        if (dCol != 0 || dRow != doubleStep)
            return true;
        if (fromRow != pawnStartRow(color, board.rows()))
            return false;
        return isPathClear(board, fromRow, fromCol, toRow, toCol);
    }

    PieceRulesRegistry::PieceRulesRegistry()
    {
        rules_ = {
            {'K', {kingShape, false}},
            {'Q', {queenShape, true}},
            {'R', {rookShape, true}},
            {'B', {bishopShape, true}},
            {'N', {knightShape, false}},
            {'P', {pawnShape, false, pawnCaptureShape, pawnContextGate}},
        };
    }

    const MoveRule *PieceRulesRegistry::find(char piece) const
    {
        auto it = rules_.find(piece);
        if (it == rules_.end())
            return nullptr;
        return &it->second;
    }

    int PieceRulesRegistry::pawnPromotionRow(char color, int totalRows) const
    {
        return (color == 'w') ? 0 : totalRows - 1;
    }
}
