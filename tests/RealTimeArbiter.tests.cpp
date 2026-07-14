#include "doctest.h"

#include "RealTimeArbiter.hpp"
#include "model/Board.hpp"
#include "BoardParser.hpp"


static pieceRules::PieceRulesRegistry registry;


namespace
{
    Board makeBoard(std::initializer_list<std::initializer_list<std::string>> rows)
    {
        RawBoard raw;
        for (const auto &row : rows)
            raw.push_back(std::vector<std::string>(row.begin(), row.end()));
        return buildBoard(raw);
    }

    // Local helper: reads back a single cell as a "colorkind" token (or
    // ".") purely for test assertions - Board itself has no such concept.
    std::string tokenAt(const Board &b, int row, int col)
    {
        const Piece *p = b.pieceAt(Position{row, col});
        if (!p) return ".";
        return std::string(1, p->color) + std::string(1, p->kind);
    }

    // Local helper: same idea as tokenAt, but for a Piece snapshot
    // returned directly from resolveMoves' captured list.
    std::string token(const Piece &p)
    {
        return std::string(1, p.color) + std::string(1, p.kind);
    }

    PieceMove makeMove(const Board &b, int fromRow, int fromCol, int toRow, int toCol, long startMs, long durationMs)
    {
        PieceMove m;
        m.fromRow = fromRow; m.fromCol = fromCol;
        m.toRow = toRow;     m.toCol = toCol;
        m.startMs = startMs; m.durationMs = durationMs;
        const Piece *p = b.pieceAt(Position{fromRow, fromCol});
        m.pieceId = p ? p->id : -1;
        return m;
    }

    JumpMove makeJump(const Board &b, int row, int col, long startMs, long durationMs)
    {
        JumpMove j;
        j.row = row; j.col = col;
        j.startMs = startMs; j.durationMs = durationMs;
        const Piece *p = b.pieceAt(Position{row, col});
        j.pieceId = p ? p->id : -1;
        return j;
    }
}

TEST_CASE("is_piece_in_flight_matches_origin_square_state_during_move")
{
    // Why this matters: the origin square should be treated as occupied by an in-flight piece until the move lands, matching the current engine flow.
    // Arrange
    Board b = makeBoard({{"wR", ".", ".", "."}});
    RealTimeArbiter arbiter;
    PieceMove m = makeMove(b, 0, 0, 0, 3, 0, 1000);
    arbiter.startMotion(b, m);

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
    Board b = makeBoard({{"wR", ".", "."}});
    RealTimeArbiter arbiter;
    PieceMove m = makeMove(b, 0, 0, 0, 1, 0, 1000);
    arbiter.startMotion(b, m);
    CHECK(arbiter.hasActiveMotion());
}

TEST_CASE("resolveMoves lands the piece at destination when duration expires")
{
    // Arrange: יוצרים לוח שבו הכלי wR יצא מ-(0,0) ונמצא בדרך ל-(0,2)
    Board b = makeBoard({{"wR", ".", "."}});
    RealTimeArbiter arbiter;
    PieceMove m = makeMove(b, 0, 0, 0, 2, 0, 500);
    arbiter.startMotion(b, m);

    // נקבע שהזמן הנוכחי הוא 600ms (עבר את ה-500ms של משך התנועה)
    // Act
    arbiter.resolveMoves(b, 600, registry);

    // Assert: המהלך היה צריך להסתיים
    CHECK_FALSE(arbiter.hasActiveMotion()); // תור התנועות התרוקן
    CHECK(tokenAt(b, 0, 2) == "wR");        // הכלי נחת בהצלחה ביעד
}

TEST_CASE("white pawn promoted to queen on arrival at row 0")
{
    Board b = makeBoard({{"."}, {"wP"}});
    RealTimeArbiter arbiter;
    PieceMove m = makeMove(b, 1, 0, 0, 0, 0, 500);
    arbiter.startMotion(b, m);

    arbiter.resolveMoves(b, 500, registry);

    CHECK(tokenAt(b, 0, 0) == "wQ");
}

TEST_CASE("black pawn promoted to queen on arrival at last row")
{
    Board b = makeBoard({{"."}, {"."}, {"."}, {"."}, {"."}, {"."}, {"bP"}, {"."}});
    RealTimeArbiter arbiter;
    PieceMove m = makeMove(b, 6, 0, 7, 0, 0, 500);
    arbiter.startMotion(b, m);

    arbiter.resolveMoves(b, 500, registry);

    CHECK(tokenAt(b, 7, 0) == "bQ");
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
    PieceMove m = makeMove(b, 6, 0, 5, 0, 0, 500);
    arbiter.startMotion(b, m);

    arbiter.resolveMoves(b, 500, registry);

    CHECK(tokenAt(b, 5, 0) == "wP");
}

TEST_CASE("arrival onto a friendly-occupied destination leaves both pieces in place") {
    Board b = makeBoard({{"wR", "wP"}});
    RealTimeArbiter arbiter;
    PieceMove m = makeMove(b, 0, 0, 0, 1, 0, 500);
    arbiter.startMotion(b, m);

    std::vector<Piece> captured = arbiter.resolveMoves(b, 500, registry);

    CHECK(captured.empty());
    CHECK(tokenAt(b, 0, 0) == "wR"); // move failed - mover stayed at origin
    CHECK(tokenAt(b, 0, 1) == "wP"); // friendly piece undisturbed
}

TEST_CASE("non-pawn piece is never modified by promotion check")
{
    Board b = makeBoard({{"."}, {"wR"}});
    RealTimeArbiter arbiter;
    PieceMove m = makeMove(b, 1, 0, 0, 0, 0, 500);
    arbiter.startMotion(b, m);

    arbiter.resolveMoves(b, 500, registry);

    CHECK(tokenAt(b, 0, 0) == "wR");
}

TEST_CASE("pawn promotion applies when arrival is a capture")
{
    Board b = makeBoard({{"bK"}, {"wP"}});
    RealTimeArbiter arbiter;
    PieceMove m = makeMove(b, 1, 0, 0, 0, 0, 500);
    arbiter.startMotion(b, m);

    std::vector<Piece> captured = arbiter.resolveMoves(b, 500, registry);

    CHECK(captured.size() == 1);
    CHECK(token(captured[0]) == "bK");
    CHECK(tokenAt(b, 0, 0) == "wQ");
}
TEST_CASE("hasActiveJumpAt is true right after startJump and false once it lands untouched")
{
    Board b = makeBoard({{"bR"}});
    RealTimeArbiter arbiter;
    JumpMove j = makeJump(b, 0, 0, 0, 1000);
    arbiter.startJump(b, j);

    CHECK(arbiter.hasActiveJumpAt(0, 0));

    arbiter.resolveMoves(b, 1000, registry);

    CHECK_FALSE(arbiter.hasActiveJumpAt(0, 0));
    CHECK(tokenAt(b, 0, 0) == "bR"); // the piece never moved, board untouched
}

TEST_CASE("airborne piece captures an enemy that arrives during the jump window")
{
    // Locks the core "defense" rule: the arriving enemy is removed, the
    // jumper stays exactly where it was.
    Board b = makeBoard({{"bR", "wR"}});
    RealTimeArbiter arbiter;
    JumpMove j = makeJump(b, 0, 0, 0, 1000);
    arbiter.startJump(b, j);

    PieceMove m = makeMove(b, 0, 1, 0, 0, 0, 500);
    arbiter.startMotion(b, m);

    std::vector<Piece> captured = arbiter.resolveMoves(b, 500, registry);

    REQUIRE(captured.size() == 1);
    CHECK(token(captured[0]) == "wR");
    CHECK(tokenAt(b, 0, 0) == "bR"); // the airborne piece did not move
    CHECK(tokenAt(b, 0, 1) == "."); // the arriving enemy was removed from its origin
}

TEST_CASE("a jump does not defend against an arriving friendly piece")
{
    // Locks: "only an enemy jump defends" - same color just blocks normally.
    Board b = makeBoard({{"bR", "bQ"}});
    RealTimeArbiter arbiter;
    JumpMove j = makeJump(b, 0, 0, 0, 1000);
    arbiter.startJump(b, j);

    PieceMove m = makeMove(b, 0, 1, 0, 0, 0, 500);
    arbiter.startMotion(b, m);

    std::vector<Piece> captured = arbiter.resolveMoves(b, 500, registry);

    CHECK(captured.empty());
    CHECK(tokenAt(b, 0, 0) == "bR"); // still the jumping rook, undisturbed
    CHECK(tokenAt(b, 0, 1) == "bQ"); // the friendly mover stayed at its origin - move failed
}

TEST_CASE("a jump that receives no enemy lands normally with the board unchanged")
{
    Board b = makeBoard({{"bR"}});
    RealTimeArbiter arbiter;
    JumpMove j = makeJump(b, 0, 0, 0, 1000);
    arbiter.startJump(b, j);

    arbiter.resolveMoves(b, 1000, registry);

    CHECK_FALSE(arbiter.hasActiveJumpAt(0, 0));
    CHECK(tokenAt(b, 0, 0) == "bR");
}
