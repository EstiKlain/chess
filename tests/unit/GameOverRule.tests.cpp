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

TEST_CASE("winnerFromCaptured returns nullopt for empty captured list")
{
    CHECK_FALSE(winnerFromCaptured({}).has_value());
}

TEST_CASE("winnerFromCaptured returns nullopt for non-king captures")
{
    CHECK_FALSE(winnerFromCaptured({makePiece('w', 'P'), makePiece('b', 'R')}).has_value());
}

TEST_CASE("winnerFromCaptured returns 'b' when the white king is captured")
{
    const auto winner = winnerFromCaptured({makePiece('w', 'P'), makePiece('w', 'K')});
    REQUIRE(winner.has_value());
    CHECK(*winner == 'b');
}

TEST_CASE("winnerFromCaptured returns 'w' when the black king is captured")
{
    const auto winner = winnerFromCaptured({makePiece('b', 'K')});
    REQUIRE(winner.has_value());
    CHECK(*winner == 'w');
}
