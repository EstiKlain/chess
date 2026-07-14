// NOTE: written against doctest, matching the *.tests.cpp naming
// convention already used in your rules/realtime unit tests.
#include "doctest.h"

#include "view/render/BoardGeometry.hpp"

TEST_CASE("boardPixelSize scales with rows/cols/cellSize -- never hard-coded to 8x8")
{
    auto size8 = BoardGeometry::boardPixelSize(8, 8, 100);
    CHECK(size8.width == 800);
    CHECK(size8.height == 800);

    auto sizeOther = BoardGeometry::boardPixelSize(10, 6, 50);
    CHECK(sizeOther.width == 300);
    CHECK(sizeOther.height == 500);
}

TEST_CASE("cellRect returns the correct top-left pixel origin")
{
    auto r = BoardGeometry::cellRect(2, 3, 100);
    CHECK(r.x == 300);
    CHECK(r.y == 200);
    CHECK(r.w == 100);
    CHECK(r.h == 100);

    auto r0 = BoardGeometry::cellRect(0, 0, 100);
    CHECK(r0.x == 0);
    CHECK(r0.y == 0);
}

TEST_CASE("isLightSquare alternates in the standard chessboard pattern")
{
    CHECK(BoardGeometry::isLightSquare(0, 0));
    CHECK_FALSE(BoardGeometry::isLightSquare(0, 1));
    CHECK_FALSE(BoardGeometry::isLightSquare(1, 0));
    CHECK(BoardGeometry::isLightSquare(1, 1));
    CHECK(BoardGeometry::isLightSquare(7, 7));
}
