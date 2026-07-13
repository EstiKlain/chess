#include "doctest.h"

#include "RealTimeArbiter.hpp"
#include "Board.hpp"


static pieceRules::PieceRulesRegistry registry;


namespace
{
    Board makeBoard(std::initializer_list<std::initializer_list<std::string>> rows)
    {
        Board b;
        for (const auto &row : rows)
        {
            b.grid.push_back(std::vector<std::string>(row.begin(), row.end()));
        }
        return b;
    }
}

TEST_CASE("is_piece_in_flight_matches_origin_square_state_during_move")
{
    // Why this matters: the origin square should be treated as occupied by an in-flight piece until the move lands, matching the current engine flow.
    // Arrange
    RealTimeArbiter arbiter;
    PieceMove m;
    m.fromRow = 0;
    m.fromCol = 0;
    m.toRow = 0;
    m.toCol = 3;
    m.startMs = 0;
    m.durationMs = 1000;
    m.piece = "wR";
    arbiter.startMotion(m);

    // Act / Assert
    CHECK(arbiter.isPieceInFlight(0, 0));
    CHECK_FALSE(arbiter.isPieceInFlight(0, 3));
}

TEST_CASE("hasActiveMotion is false when activeMoves is empty")
{
    RealTimeArbiter arbiter;
    CHECK_FALSE(arbiter.hasActiveMotion());
}

TEST_CASE("hasActiveMotion is true when a move is in flight")
{
    RealTimeArbiter arbiter;
    PieceMove m;
    m.fromRow = 0;
    m.fromCol = 0;
    m.toRow = 0;
    m.toCol = 1;
    m.startMs = 0;
    m.durationMs = 1000;
    m.piece = "wR";
    arbiter.startMotion(m);
    CHECK(arbiter.hasActiveMotion());
}

TEST_CASE("resolveMoves lands the piece at destination when duration expires")
{
    // Arrange: יוצרים לוח שבו הכלי wR יצא מ-(0,0) ונמצא בדרך ל-(0,2)
    Board b = makeBoard({{".", ".", "."}});
    RealTimeArbiter arbiter;
    PieceMove m;
    m.fromRow = 0;
    m.fromCol = 0;
    m.toRow = 0;
    m.toCol = 2;
    m.startMs = 0;
    m.durationMs = 500;
    m.piece = "wR";
    arbiter.startMotion(m);

    // נקבע שהזמן הנוכחי הוא 600ms (עבר את ה-500ms של משך התנועה)
    // Act
    arbiter.resolveMoves(b, 600 , registry);

    // Assert: המהלך היה צריך להסתיים
    CHECK_FALSE(arbiter.hasActiveMotion()); // תור התנועות התרוקן
    CHECK(b.grid[0][2] == "wR");            // הכלי נחת בהצלחה ביעד
}

TEST_CASE("white pawn promoted to queen on arrival at row 0")
{
    Board b = makeBoard({{"."}, {"wP"}});
    RealTimeArbiter arbiter;
    PieceMove m;
    m.fromRow = 1;
    m.fromCol = 0;
    m.toRow = 0;
    m.toCol = 0;
    m.startMs = 0;
    m.durationMs = 500;
    m.piece = "wP";
    arbiter.startMotion(m);

    arbiter.resolveMoves(b, 500, registry);

    CHECK(b.grid[0][0] == "wQ");
}

TEST_CASE("black pawn promoted to queen on arrival at last row")
{
    Board b = makeBoard({{"."}, {"."}, {"."}, {"."}, {"."}, {"."}, {"."}, {"."}});
    RealTimeArbiter arbiter;
    PieceMove m;
    m.fromRow = 6;
    m.fromCol = 0;
    m.toRow = 7;
    m.toCol = 0;
    m.startMs = 0;
    m.durationMs = 500;
    m.piece = "bP";
    arbiter.startMotion(m);

    arbiter.resolveMoves(b, 500, registry);

    CHECK(b.grid[7][0] == "bQ");
}

TEST_CASE("pawn arriving at non-promotion row remains a pawn")
{
    Board b = makeBoard({
        {".", ".", "."},
        {".", ".", "."},
        {".", ".", "."},
        {".", ".", "."},
        {".", ".", "."},
        {".", ".", "."},
        {"wP", ".", "."},
        {".", ".", "."}
    });
    RealTimeArbiter arbiter;
    PieceMove m;
    m.fromRow = 6;
    m.fromCol = 0;
    m.toRow = 5;
    m.toCol = 0;
    m.startMs = 0;
    m.durationMs = 500;
    m.piece = "wP";
    arbiter.startMotion(m);

    arbiter.resolveMoves(b, 500, registry);

    CHECK(b.grid[5][0] == "wP");
}

TEST_CASE("non-pawn piece is never modified by promotion check")
{
    Board b = makeBoard({{"."}, {"wR"}});
    RealTimeArbiter arbiter;
    PieceMove m;
    m.fromRow = 1;
    m.fromCol = 0;
    m.toRow = 0;
    m.toCol = 0;
    m.startMs = 0;
    m.durationMs = 500;
    m.piece = "wR";
    arbiter.startMotion(m);

    arbiter.resolveMoves(b, 500, registry);

    CHECK(b.grid[0][0] == "wR");
}

TEST_CASE("pawn promotion applies when arrival is a capture")
{
    Board b = makeBoard({{"bK"}, {"wP"}});
    RealTimeArbiter arbiter;
    PieceMove m;
    m.fromRow = 1;
    m.fromCol = 0;
    m.toRow = 0;
    m.toCol = 0;
    m.startMs = 0;
    m.durationMs = 500;
    m.piece = "wP";
    arbiter.startMotion(m);

    std::vector<std::string> captured = arbiter.resolveMoves(b, 500, registry);

    CHECK(captured.size() == 1);
    CHECK(captured[0] == "bK");
    CHECK(b.grid[0][0] == "wQ");
}
