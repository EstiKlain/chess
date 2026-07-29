#include "rules/PromotionRule.hpp"

char promotedKind(char kind, char color, int arrivalRow, int totalRows,
                   const pieceRules::PieceRulesRegistry &registry)
{
    if (kind == 'P' && arrivalRow == registry.pawnPromotionRow(color, totalRows))
        return 'Q';
    return kind;
}
