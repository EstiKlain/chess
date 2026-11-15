#include "rules/PieceRules.hpp"
#include <cassert>
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

    std::vector<Position> straightLinePathCells(int fromRow, int fromCol, int toRow, int toCol)
    {
        int stepRow = sign(toRow - fromRow);
        int stepCol = sign(toCol - fromCol);

        std::vector<Position> cells;
        int r = fromRow + stepRow, c = fromCol + stepCol;
        while (true)
        {
            cells.push_back(Position{r, c});
            if (r == toRow && c == toCol)
                break;
            r += stepRow;
            c += stepCol;
        }
        return cells;
    }

    bool isPathClearAlong(const Board &board, const std::vector<Position> &cells)
    {
        // Every cell except the last (the destination) must be empty -
        // matches isPathClear's existing semantics of never checking the
        // destination cell itself.
        for (size_t i = 0; i + 1 < cells.size(); ++i)
            if (board.pieceAt(cells[i]) != nullptr)
                return false;
        return true;
    }

    bool isPathClear(const Board &board, int fromRow, int fromCol, int toRow, int toCol)
    {
        return isPathClearAlong(board, straightLinePathCells(fromRow, fromCol, toRow, toCol));
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
            {'Q', {queenShape, true, nullptr, nullptr, straightLinePathCells}},
            {'R', {rookShape, true, nullptr, nullptr, straightLinePathCells}},
            {'B', {bishopShape, true, nullptr, nullptr, straightLinePathCells}},
            {'N', {knightShape, false}},
            {'P', {pawnShape, false, pawnCaptureShape, pawnContextGate, straightLinePathCells}},
        };

        // One-directional only: slides=true means checkPieceShape's
        // blocked_path check is gated on `rule->slides && rule->pathCells`
        // (Movement.cpp) - forgetting pathCells on a new sliding piece would
        // make that check silently never fire (the piece could hop over
        // others with no error). The reverse is NOT required: 'P' above has
        // slides=false (its own path-blocking is handled separately, via
        // pawnContextGate) yet still defines pathCells, deliberately, so
        // RealTimeArbiter can detect mid-flight collisions along a pawn's
        // double-step - that combination is correct, not a bug, so this
        // assert must not require slides == (pathCells != nullptr).
        for (const auto &[kind, rule] : rules_)
            if (rule.slides)
                assert(rule.pathCells && "a sliding piece must define pathCells, or blocked_path checking silently never fires");
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
