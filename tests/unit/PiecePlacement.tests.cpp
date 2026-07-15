// Pure CSV-parsing logic, no OpenCV/window involved -- each test writes its
// own tiny scratch CSV so nothing here depends on the real assets/board.csv.
#include "doctest.h"

#include "view/render/PiecePlacement.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <stdexcept>

namespace
{
    struct TempCsv
    {
        std::string path;
        TempCsv(const std::string &content, const std::string &name) : path(name)
        {
            std::ofstream out(path);
            out << content;
        }
        ~TempCsv() { std::remove(path.c_str()); }
    };

    bool contains(const std::vector<PiecePlacement> &v, const std::string &code, int row, int col)
    {
        return std::any_of(v.begin(), v.end(), [&](const PiecePlacement &p)
                            { return p.pieceCode == code && p.row == row && p.col == col; });
    }
}

TEST_CASE("loadOpeningFromCsv places pieces at the correct row/col")
{
    TempCsv csv("RB,,,,,,,RB\n,,,,,,,\n", "test_opening_basic.csv");

    const auto placements = loadOpeningFromCsv(csv.path);

    REQUIRE(placements.size() == 2);
    CHECK(contains(placements, "RB", 0, 0));
    CHECK(contains(placements, "RB", 0, 7));
}

TEST_CASE("loadOpeningFromCsv skips empty cells instead of creating placeholders")
{
    TempCsv csv("QW,,,\n", "test_opening_empty_cells.csv");

    const auto placements = loadOpeningFromCsv(csv.path);

    REQUIRE(placements.size() == 1);
    CHECK(placements[0].pieceCode == "QW");
    CHECK(placements[0].row == 0);
    CHECK(placements[0].col == 0);
}

TEST_CASE("loadOpeningFromCsv strips trailing \\r from Windows line endings")
{
    TempCsv csv("PW,,\r\n,,PB\r\n", "test_opening_crlf.csv");

    const auto placements = loadOpeningFromCsv(csv.path);

    REQUIRE(placements.size() == 2);
    CHECK(contains(placements, "PW", 0, 0));
    CHECK(contains(placements, "PB", 1, 2));
}

TEST_CASE("loadOpeningFromCsv throws when the file doesn't exist")
{
    CHECK_THROWS_AS(loadOpeningFromCsv("no_such_file_ever.csv"), std::runtime_error);
}