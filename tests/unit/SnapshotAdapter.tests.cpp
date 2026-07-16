// SnapshotAdapter is pure translation: GameSnapshot's (kind, color) ->
// PiecePlacement's pieceCode string convention. No engine, no board, no
// Img/OpenCV needed to test this at all - it's built entirely from a
// hand-made GameSnapshot, the way the plan intends.
#include "doctest.h"

#include "view/render/SnapshotAdapter.hpp"

TEST_CASE("toPlacements converts kind+color into the pieceCode convention (kind, then uppercase color)")
{
    GameSnapshot s;
    s.rows = 8;
    s.cols = 8;
    s.gameOver = false;
    s.pieces = {
        PieceSnapshot{1, 'w', 'Q', 0, 3},
        PieceSnapshot{2, 'b', 'P', 6, 4},
    };

    const auto placements = SnapshotAdapter::toPlacements(s);

    REQUIRE(placements.size() == 2);
    CHECK(placements[0].pieceCode == "QW");
    CHECK(placements[0].row == 0);
    CHECK(placements[0].col == 3);
    CHECK(placements[1].pieceCode == "PB");
    CHECK(placements[1].row == 6);
    CHECK(placements[1].col == 4);
}

TEST_CASE("toPlacements returns an empty list for an empty snapshot")
{
    GameSnapshot s;
    s.rows = 8;
    s.cols = 8;
    CHECK(SnapshotAdapter::toPlacements(s).empty());
}
