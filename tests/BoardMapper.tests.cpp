#include "doctest.h"

#include "BoardMapper.hpp"
#include "config.hpp"

TEST_CASE("pixel_to_cell_maps_each_cell_range")
{
    CHECK(BoardMapper::pixelToCell(0, 0, 4, 4) == Position{0, 0});
    CHECK(BoardMapper::pixelToCell(99, 99, 4, 4) == Position{0, 0});
    CHECK(BoardMapper::pixelToCell(100, 0, 4, 4) == Position{0, 1});
    CHECK(BoardMapper::pixelToCell(0, 100, 4, 4) == Position{1, 0});
    CHECK(BoardMapper::pixelToCell(199, 199, 4, 4) == Position{1, 1});
}

TEST_CASE("pixel_to_cell_returns_nullopt_outside_board_bounds")
{
    CHECK_FALSE(BoardMapper::pixelToCell(-1, 0, 4, 4).has_value());
    CHECK_FALSE(BoardMapper::pixelToCell(0, -1, 4, 4).has_value());
    CHECK_FALSE(BoardMapper::pixelToCell(400, 0, 4, 4).has_value());
    CHECK_FALSE(BoardMapper::pixelToCell(0, 400, 4, 4).has_value());
    CHECK_FALSE(BoardMapper::pixelToCell(400, 400, 4, 4).has_value());
}
