#include "doctest.h"

#include "rules/PromotionRule.hpp"

namespace
{
    pieceRules::PieceRulesRegistry registry;
}

TEST_CASE("promotedKind promotes a white pawn arriving at row 0 to a queen")
{
    CHECK(promotedKind('P', 'w', 0, 8, registry) == 'Q');
}

TEST_CASE("promotedKind promotes a black pawn arriving at the last row to a queen")
{
    CHECK(promotedKind('P', 'b', 7, 8, registry) == 'Q');
}

TEST_CASE("promotedKind leaves a pawn unchanged when it hasn't reached its promotion row")
{
    CHECK(promotedKind('P', 'w', 3, 8, registry) == 'P');
}

TEST_CASE("promotedKind leaves a non-pawn kind unchanged even at row 0")
{
    CHECK(promotedKind('R', 'w', 0, 8, registry) == 'R');
    CHECK(promotedKind('Q', 'w', 0, 8, registry) == 'Q');
}
