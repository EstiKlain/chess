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

    CHECK(isMoveLegal(b, move, 'R'));
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

    CHECK_FALSE(isMoveLegal(b, move, 'R'));
}
