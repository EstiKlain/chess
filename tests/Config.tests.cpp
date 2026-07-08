#include "doctest.h"

#include "config.hpp"

TEST_CASE("statsFor returns known speeds for standard pieces") {
    CHECK(config::statsFor('Q').speedCellsPerSec == doctest::Approx(4.0));
    CHECK(config::statsFor('R').speedCellsPerSec == doctest::Approx(3.0));
    CHECK(config::statsFor('B').speedCellsPerSec == doctest::Approx(3.0));
    CHECK(config::statsFor('N').speedCellsPerSec == doctest::Approx(3.5));
    CHECK(config::statsFor('K').speedCellsPerSec == doctest::Approx(3.0));
    CHECK(config::statsFor('P').speedCellsPerSec == doctest::Approx(2.0));
}

TEST_CASE("statsFor defaults unknown pieces to zero speed") {
    CHECK(config::statsFor('X').speedCellsPerSec == doctest::Approx(0.0));
}

TEST_CASE("shape helpers classify king moves") {
    CHECK(config::kingShape(1, 0, 'w'));
    CHECK(config::kingShape(1, 1, 'w'));
    CHECK_FALSE(config::kingShape(0, 0, 'w'));
    CHECK_FALSE(config::kingShape(2, 0, 'w'));
}

TEST_CASE("shape helpers classify rook moves") {
    CHECK(config::rookShape(0, 5, 'w'));
    CHECK(config::rookShape(5, 0, 'w'));
    CHECK_FALSE(config::rookShape(0, 0, 'w'));
    CHECK_FALSE(config::rookShape(2, 2, 'w'));
}

TEST_CASE("shape helpers classify bishop moves") {
    CHECK(config::bishopShape(3, 3, 'w'));
    CHECK(config::bishopShape(-2, 2, 'w'));
    CHECK_FALSE(config::bishopShape(0, 0, 'w'));
    CHECK_FALSE(config::bishopShape(2, 3, 'w'));
}

TEST_CASE("shape helpers classify queen moves as rook or bishop") {
    CHECK(config::queenShape(0, 4, 'w'));
    CHECK(config::queenShape(4, 4, 'w'));
    CHECK_FALSE(config::queenShape(1, 2, 'w'));
}

TEST_CASE("shape helpers classify knight moves") {
    CHECK(config::knightShape(1, 2, 'w'));
    CHECK(config::knightShape(2, 1, 'w'));
    CHECK_FALSE(config::knightShape(1, 1, 'w'));
    CHECK_FALSE(config::knightShape(2, 2, 'w'));
}

TEST_CASE("pawnForwardDir depends on color") {
    CHECK(config::pawnForwardDir('w') == -1);
    CHECK(config::pawnForwardDir('b') == 1);
}

TEST_CASE("shape helpers classify pawn moves and captures separately") {
    CHECK(config::pawnShape(-1, 0, 'w'));
    CHECK_FALSE(config::pawnShape(-1, 1, 'w'));
    CHECK(config::pawnCaptureShape(-1, 1, 'w'));
    CHECK(config::pawnCaptureShape(-1, -1, 'w'));
    CHECK_FALSE(config::pawnCaptureShape(-1, 0, 'w'));
}

TEST_CASE("moveShapes registers a rule for every standard piece") {
    for (char piece : {'K', 'Q', 'R', 'B', 'N', 'P'}) {
        CHECK(config::moveShapes.count(piece) == 1);
    }
}
