#include "doctest.h"

#include "rules/CaptureRule.hpp"

TEST_CASE("capturesOnArrival is true when the colors differ")
{
    CHECK(capturesOnArrival('w', 'b'));
    CHECK(capturesOnArrival('b', 'w'));
}

TEST_CASE("capturesOnArrival is false when the colors match")
{
    CHECK_FALSE(capturesOnArrival('w', 'w'));
    CHECK_FALSE(capturesOnArrival('b', 'b'));
}
