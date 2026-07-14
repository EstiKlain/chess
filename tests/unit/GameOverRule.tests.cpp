#include "doctest.h"

#include "rules/GameOverRule.hpp"
#include "model/Piece.hpp"

namespace {
    Piece makePiece(char color, char kind) {
        Piece p;
        p.id = 1;
        p.color = color;
        p.kind = kind;
        p.cell = Position{0, 0};
        p.state = PieceState::Captured;
        return p;
    }
}

TEST_CASE("isGameOver returns false for empty captured list")
{
    CHECK_FALSE(isGameOver({}));
}

TEST_CASE("isGameOver returns false for non-king captures")
{
    CHECK_FALSE(isGameOver({makePiece('w', 'P'), makePiece('b', 'R')}));
}

TEST_CASE("isGameOver returns true when a king is captured")
{
    CHECK(isGameOver({makePiece('w', 'P'), makePiece('b', 'K')}));
}
