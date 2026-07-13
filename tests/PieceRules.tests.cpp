#include "doctest.h"
#include "PieceRules.hpp"
#include "Board.hpp"
#include "BoardParser.hpp"

TEST_CASE("shape helpers classify king moves") {
    CHECK(pieceRules::kingShape(1, 0, 'w'));
    CHECK(pieceRules::kingShape(1, 1, 'w'));
    CHECK_FALSE(pieceRules::kingShape(0, 0, 'w'));
    CHECK_FALSE(pieceRules::kingShape(2, 0, 'w'));
}

TEST_CASE("shape helpers classify rook moves") {
    CHECK(pieceRules::rookShape(0, 5, 'w'));
    CHECK(pieceRules::rookShape(5, 0, 'w'));
    CHECK_FALSE(pieceRules::rookShape(0, 0, 'w'));
    CHECK_FALSE(pieceRules::rookShape(2, 2, 'w'));
}

TEST_CASE("shape helpers classify bishop moves") {
    CHECK(pieceRules::bishopShape(3, 3, 'w'));
    CHECK(pieceRules::bishopShape(-2, 2, 'w'));
    CHECK_FALSE(pieceRules::bishopShape(0, 0, 'w'));
    CHECK_FALSE(pieceRules::bishopShape(2, 3, 'w'));
}

TEST_CASE("shape helpers classify queen moves as rook or bishop") {
    CHECK(pieceRules::queenShape(0, 4, 'w'));
    CHECK(pieceRules::queenShape(4, 4, 'w'));
    CHECK_FALSE(pieceRules::queenShape(1, 2, 'w'));
}

TEST_CASE("shape helpers classify knight moves") {
    CHECK(pieceRules::knightShape(1, 2, 'w'));
    CHECK(pieceRules::knightShape(2, 1, 'w'));
    CHECK_FALSE(pieceRules::knightShape(1, 1, 'w'));
    CHECK_FALSE(pieceRules::knightShape(2, 2, 'w'));
}

TEST_CASE("pawnForwardDir depends on color") {
    CHECK(pieceRules::pawnForwardDir('w') == -1);
    CHECK(pieceRules::pawnForwardDir('b') == 1);
}

TEST_CASE("shape helpers classify pawn moves and captures separately") {
    CHECK(pieceRules::pawnShape(-1, 0, 'w'));
    CHECK(pieceRules::pawnShape(-2, 0, 'w'));
    CHECK_FALSE(pieceRules::pawnShape(-1, 1, 'w'));
    CHECK_FALSE(pieceRules::pawnShape(-2, 2, 'w'));
    CHECK(pieceRules::pawnShape(1, 0, 'b'));
    CHECK(pieceRules::pawnShape(2, 0, 'b'));
    CHECK_FALSE(pieceRules::pawnShape(2, 2, 'b'));
    CHECK(pieceRules::pawnCaptureShape(-1, 1, 'w'));
    CHECK(pieceRules::pawnCaptureShape(-1, -1, 'w'));
    CHECK_FALSE(pieceRules::pawnCaptureShape(-1, 0, 'w'));
}

TEST_CASE("pawnStartRow returns canonical start row for each color on 8 rows") {
    CHECK(pieceRules::pawnStartRow('w', 8) == 6);
    CHECK(pieceRules::pawnStartRow('b', 8) == 1);
}

TEST_CASE("pawnPromotionRow returns canonical far row for each color on 8 rows") {
    pieceRules::PieceRulesRegistry registry;
    CHECK(registry.pawnPromotionRow('w', 8) == 0);
    CHECK(registry.pawnPromotionRow('b', 8) == 7);
}

TEST_CASE("registry registers a rule for every standard piece") {
    pieceRules::PieceRulesRegistry registry;
    for (char piece : {'K', 'Q', 'R', 'B', 'N', 'P'}) {
        CHECK(registry.find(piece) != nullptr);
    }
}

TEST_CASE("sign returns -1, 0 or 1") {
    CHECK(pieceRules::sign(5) == 1);
    CHECK(pieceRules::sign(-5) == -1);
    CHECK(pieceRules::sign(0) == 0);
}

TEST_CASE("isPathClear allows adjacent squares with nothing in between") {
    Board b = parseBoard({"wK .", ". bK"});
    CHECK(pieceRules::isPathClear(b, 0, 0, 1, 1));
}

TEST_CASE("isPathClear returns true when all intermediate squares are empty") {
    Board b = parseBoard({"wR . . bR"});
    CHECK(pieceRules::isPathClear(b, 0, 0, 0, 3));
}

TEST_CASE("isPathClear returns false when a piece blocks the path") {
    Board b = parseBoard({"wR . wP bR"});
    CHECK_FALSE(pieceRules::isPathClear(b, 0, 0, 0, 3));
}

TEST_CASE("isPathClear works along diagonals") {
    Board b = parseBoard({
        "wB . . .",
        ". . . .",
        ". . bP .",
        ". . . bB"
    });
    CHECK_FALSE(pieceRules::isPathClear(b, 0, 0, 3, 3));
}