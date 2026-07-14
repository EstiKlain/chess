#include "doctest.h"

#include "config.hpp"

TEST_CASE("statsFor returns known speeds for standard pieces") {
    CHECK(config::statsFor('Q').speedCellsPerSec == doctest::Approx(4.0));
    CHECK(config::statsFor('R').speedCellsPerSec == doctest::Approx(1.0));
    CHECK(config::statsFor('B').speedCellsPerSec == doctest::Approx(3.0));
    CHECK(config::statsFor('N').speedCellsPerSec == doctest::Approx(3.5));
    CHECK(config::statsFor('K').speedCellsPerSec == doctest::Approx(3.0));
    CHECK(config::statsFor('P').speedCellsPerSec == doctest::Approx(2.0));
}

TEST_CASE("statsFor defaults unknown pieces to zero speed") {
    CHECK(config::statsFor('X').speedCellsPerSec == doctest::Approx(0.0));
}
