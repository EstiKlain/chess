#include "doctest.h"

#include "view/canvas/WindowMapper.hpp"

TEST_CASE("scale_factor_is_one_when_window_matches_image")
{
    CHECK(WindowMapper::scaleFactor(800, 800, 800, 800) == doctest::Approx(1.0));
}

TEST_CASE("scale_factor_is_limited_by_the_tighter_axis")
{
    // Window is wider than tall relative to the image (1000x500 vs 800x800):
    // height is the tighter axis (500/800 = 0.625 < 1000/800 = 1.25).
    CHECK(WindowMapper::scaleFactor(1000, 500, 800, 800) == doctest::Approx(0.625));
}

TEST_CASE("scale_factor_scales_up_when_window_is_larger_than_image")
{
    CHECK(WindowMapper::scaleFactor(1200, 1200, 800, 800) == doctest::Approx(1.5));
}

TEST_CASE("scale_factor_returns_zero_for_degenerate_sizes")
{
    CHECK(WindowMapper::scaleFactor(0, 800, 800, 800) == doctest::Approx(0.0));
    CHECK(WindowMapper::scaleFactor(800, 0, 800, 800) == doctest::Approx(0.0));
    CHECK(WindowMapper::scaleFactor(800, 800, 0, 800) == doctest::Approx(0.0));
    CHECK(WindowMapper::scaleFactor(800, 800, 800, -1) == doctest::Approx(0.0));
}

TEST_CASE("window_to_image_is_identity_when_window_matches_image")
{
    auto p = WindowMapper::windowToImage(150, 250, 800, 800, 800, 800);
    REQUIRE(p.has_value());
    CHECK(p->x == 150);
    CHECK(p->y == 250);
}

TEST_CASE("window_to_image_scales_down_when_window_is_smaller_than_image")
{
    // scale = 400/800 = 0.5, no letterbox (both axes match ratio exactly).
    auto p = WindowMapper::windowToImage(200, 200, 400, 400, 800, 800);
    REQUIRE(p.has_value());
    CHECK(p->x == 400);
    CHECK(p->y == 400);
}

TEST_CASE("window_to_image_scales_up_when_window_is_larger_than_image")
{
    // scale = 1200/800 = 1.5, no letterbox.
    auto p = WindowMapper::windowToImage(150, 150, 1200, 1200, 800, 800);
    REQUIRE(p.has_value());
    CHECK(p->x == 100);
    CHECK(p->y == 100);
}

TEST_CASE("window_to_image_letterboxes_a_wider_than_needed_window")
{
    // image 800x800 in a 1000x800 window: scale = min(1000/800, 800/800) = 1.0,
    // so a 200px horizontal letterbox is split 100px on each side.
    auto center = WindowMapper::windowToImage(100, 0, 1000, 800, 800, 800);
    REQUIRE(center.has_value());
    CHECK(center->x == 0);
    CHECK(center->y == 0);

    // A click in the left padding (x < 100) has no corresponding image
    // pixel.
    auto padding = WindowMapper::windowToImage(50, 50, 1000, 800, 800, 800);
    CHECK_FALSE(padding.has_value());

    // A click in the right padding (past offsetX + scaledWidth = 900) is
    // also outside the rendered image.
    auto rightPadding = WindowMapper::windowToImage(950, 50, 1000, 800, 800, 800);
    CHECK_FALSE(rightPadding.has_value());
}

TEST_CASE("window_to_image_letterboxes_a_taller_than_needed_window")
{
    // image 800x800 in a 800x1000 window: scale = min(800/800, 1000/800) = 1.0,
    // so a 200px vertical letterbox is split 100px on top and bottom.
    auto top = WindowMapper::windowToImage(0, 100, 800, 1000, 800, 800);
    REQUIRE(top.has_value());
    CHECK(top->x == 0);
    CHECK(top->y == 0);

    auto padding = WindowMapper::windowToImage(50, 50, 800, 1000, 800, 800);
    CHECK_FALSE(padding.has_value());
}

TEST_CASE("window_to_image_returns_nullopt_for_degenerate_sizes")
{
    CHECK_FALSE(WindowMapper::windowToImage(10, 10, 0, 800, 800, 800).has_value());
    CHECK_FALSE(WindowMapper::windowToImage(10, 10, 800, 800, 0, 800).has_value());
}