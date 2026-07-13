#pragma once

#include <functional>
#include <map>
#include <string>

#include "Board.hpp"

namespace pieceRules
{
    using MoveShapeFn = std::function<bool(int dRow, int dCol, char color)>;

    // --- shape functions (moved verbatim from config.hpp, only the
    //     namespace changes: config:: -> pieceRules::) ---
    bool kingShape(int dRow, int dCol, char color);
    bool rookShape(int dRow, int dCol, char color);
    bool bishopShape(int dRow, int dCol, char color);
    bool queenShape(int dRow, int dCol, char color);
    bool knightShape(int dRow, int dCol, char color);
    int pawnForwardDir(char color);
    int pawnStartRow(char color, int totalRows);
    bool pawnShape(int dRow, int dCol, char color);
    bool pawnCaptureShape(int dRow, int dCol, char color);

    // --- moved from Movement.hpp (see section 1) ---
    int sign(int v);
    bool isPathClear(const Board &board, int fromRow, int fromCol, int toRow, int toCol);

    struct MoveRule
    {
        MoveShapeFn shape;
        bool slides;
        MoveShapeFn captureShape = nullptr;
        std::function<bool(const Board &, int fromRow, int fromCol, int toRow, int toCol, char color)> contextGate = nullptr;
    };

    bool pawnContextGate(const Board &board, int fromRow, int fromCol, int toRow, int toCol, char color);

    // The registry: owns the rule table privately. No global map exists
    // anywhere anymore. GameEngine owns one instance; RealTimeArbiter,
    // RuleEngine's isMoveLegal, and Movement's checkPieceShape all
    // receive a `const PieceRulesRegistry&` explicitly, they never reach
    // into a global.
    class PieceRulesRegistry
    {
    public:
        // Populates the exact same 6 default rules that exist today in
        // config::moveShapes - see the table in section 4 below. Do not
        // change any of these values.
        PieceRulesRegistry();

        // Returns nullptr if the piece char is not registered. This must
        // match today's exact behavior in checkPieceShape: an
        // unregistered piece is NOT an error, it means "always legal"
        // (see section 5, checkPieceShape).
        const MoveRule *find(char piece) const;

        // Moved from config::pawnPromotionRow. This is a method (not a
        // free function) so that RealTimeArbiter, which already needs to
        // receive a PieceRulesRegistry for consistency, doesn't have a
        // second, different way of reaching promotion-row logic.
        int pawnPromotionRow(char color, int totalRows) const;

    private:
        std::map<char, MoveRule> rules_;
    };
}
