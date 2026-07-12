#include "doctest.h"

#include "GameOverRule.hpp"

TEST_CASE("isGameOver returns false for empty captured list")
{
    CHECK_FALSE(isGameOver({}));
}

TEST_CASE("isGameOver returns false for non-king captures")
{
    CHECK_FALSE(isGameOver({"wP", "bR"}));
}

TEST_CASE("isGameOver returns true when a king is captured")
{
    CHECK(isGameOver({"wP", "bK"}));
}
