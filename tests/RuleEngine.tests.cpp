#include "doctest.h"

#include "RuleEngine.hpp"
#include "Board.hpp"
#include "GameState.hpp"

TEST_CASE("isMoveLegal delegates legal rook moves")
{
    Board b;
    b.grid = {{"wR", ".", "."}};
    PieceMove move;
    move.fromRow = 0;
    move.fromCol = 0;
    move.toRow = 0;
    move.toCol = 2;
    move.piece = "wR";

    CHECK(isMoveLegal(b, move, 'R').isValid);
}

TEST_CASE("isMoveLegal rejects bishop-shaped rook move")
{
    Board b;
    b.grid = {{"wR", ".", "."}, {".", ".", "."}};
    PieceMove move;
    move.fromRow = 0;
    move.fromCol = 0;
    move.toRow = 1;
    move.toCol = 1;
    move.piece = "wR";

    CHECK_FALSE(isMoveLegal(b, move, 'R').isValid);
}

TEST_CASE("isMoveLegal bounds checking")
{
    Board b = parseBoard({"wK .", ". ."}); // 2x2 board
    PieceMove move;
    move.fromRow = 0;
    move.fromCol = 0;
    move.piece = "wK";

    SUBCASE("outside_board - toRow too high") {
        move.toRow = 2;
        move.toCol = 0;
        auto res = isMoveLegal(b, move, 'K');
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "outside_board");
    }

    SUBCASE("outside_board - toRow too low") {
        move.toRow = -1;
        move.toCol = 0;
        auto res = isMoveLegal(b, move, 'K');
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "outside_board");
    }

    SUBCASE("outside_board - toCol too high") {
        move.toRow = 0;
        move.toCol = 2;
        auto res = isMoveLegal(b, move, 'K');
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "outside_board");
    }

    SUBCASE("outside_board - toCol too low") {
        move.toRow = 0;
        move.toCol = -1;
        auto res = isMoveLegal(b, move, 'K');
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "outside_board");
    }

    SUBCASE("forwards Movement's reason when in-bounds but illegal") {
        move.toRow = 1;
        move.toCol = 1; // Diagonal move for King is legal, so let's try something illegal
        // Wait, king diagonal IS legal. Let's use a Rook.
        move.piece = "wR";
        auto res = isMoveLegal(b, move, 'R');
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "illegal_piece_move");
    }
}
