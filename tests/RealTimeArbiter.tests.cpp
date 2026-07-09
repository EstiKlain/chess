#include "doctest.h"

#include "RealTimeArbiter.hpp"
#include "Board.hpp"
#include "GameState.hpp"

namespace
{
    GameState makeState(std::initializer_list<std::initializer_list<std::string>> rows)
    {
        GameState st;
        for (const auto &row : rows)
        {
            st.board.grid.push_back(std::vector<std::string>(row.begin(), row.end()));
        }
        return st;
    }
}

TEST_CASE("is_piece_in_flight_matches_origin_square_state_during_move")
{
    // Why this matters: the origin square should be treated as occupied by an in-flight piece until the move lands, matching the current engine flow.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    PieceMove m;
    m.fromRow = 0;
    m.fromCol = 0;
    m.toRow = 0;
    m.toCol = 3;
    m.startMs = 0;
    m.durationMs = 1000;
    m.piece = "wR";
    st.activeMoves.push_back(m);

    // Act / Assert
    CHECK(isPieceInFlight(st, 0, 0));
    CHECK_FALSE(isPieceInFlight(st, 0, 3));
}

TEST_CASE("hasActiveMotion is false when activeMoves is empty")
{
    GameState st = makeState({{"wR", "."}});
    CHECK_FALSE(hasActiveMotion(st));
}

TEST_CASE("hasActiveMotion is true when a move is in flight")
{
    GameState st = makeState({{"wR", "."}});
    PieceMove m;
    m.fromRow = 0;
    m.fromCol = 0;
    m.toRow = 0;
    m.toCol = 1;
    m.startMs = 0;
    m.durationMs = 1000;
    m.piece = "wR";
    st.activeMoves.push_back(m);
    CHECK(hasActiveMotion(st));
}

TEST_CASE("resolveMoves lands the piece at destination when duration expires")
{
    // Arrange: יוצרים לוח שבו הכלי wR יצא מ-(0,0) ונמצא בדרך ל-(0,2)
    GameState st = makeState({{".", ".", "."}});
    PieceMove m;
    m.fromRow = 0;
    m.fromCol = 0;
    m.toRow = 0;
    m.toCol = 2;
    m.startMs = 0;
    m.durationMs = 500;
    m.piece = "wR";
    st.activeMoves.push_back(m);

    // נקבע שהזמן הנוכחי הוא 600ms (עבר את ה-500ms של משך התנועה)
    st.elapsedMs = 600;

    // Act
    resolveMoves(st);

    // Assert: המהלך היה צריך להסתיים
    CHECK(st.activeMoves.empty());      // תור התנועות התרוקן
    CHECK(st.board.grid[0][2] == "wR"); // הכלי נחת בהצלחה ביעד
}