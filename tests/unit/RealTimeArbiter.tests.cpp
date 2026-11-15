#include "doctest.h"

#include "realtime/RealTimeArbiter.hpp"
#include "model/Board.hpp"
#include "io/BoardParser.hpp"
#include "config.hpp"

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

        // Mirrors GameEngine::requestMove: the path is pre-computed here,
        // from the mover's own MoveRule::pathCells, not inside the arbiter.
        if (const pieceRules::MoveRule *rule = p ? registry.find(p->kind) : nullptr; rule && rule->pathCells)
            m.path = rule->pathCells(fromRow, fromCol, toRow, toCol);

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

TEST_CASE("no piece is in flight when activeMoves is empty")
{
    RealTimeArbiter arbiter;
    CHECK_FALSE(arbiter.isPieceInFlight(0, 0));
}

TEST_CASE("isPieceInFlight is true when a move is in flight")
{
    Board b = makeBoard({{"wR", ".", "."}});
    RealTimeArbiter arbiter;
    PieceMove m = makeMove(b, 0, 0, 0, 1, 0, 1000);
    arbiter.startMotion(b, m);
    CHECK(arbiter.isPieceInFlight(0, 0));
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
    CHECK_FALSE(arbiter.isPieceInFlight(0, 0)); // המהלך הוסר מתור התנועות הפעילות
    CHECK(tokenAt(b, 0, 2) == "wR");            // הכלי נחת בהצלחה ביעד
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

// --- Iteration E: activeMoveForPiece ------------------------------------
// Read-only lookup GameEngine::snapshot() uses to fill MotionSnapshot.
// Must return by value (a copy) - never a reference into activeMoves_ -
// so nothing about RealTimeArbiter/PieceMove leaks past the engine
// boundary. See RealTimeArbiter.hpp.

TEST_CASE("activeMoveForPiece returns nullopt for a piece with no move in flight")
{
    Board b = makeBoard({{"wR", ".", "."}});
    RealTimeArbiter arbiter;
    const Piece *rook = b.pieceAt(Position{0, 0});
    REQUIRE(rook != nullptr);

    CHECK_FALSE(arbiter.activeMoveForPiece(rook->id).has_value());
}

TEST_CASE("activeMoveForPiece returns nullopt for an id that doesn't exist at all")
{
    RealTimeArbiter arbiter;
    CHECK_FALSE(arbiter.activeMoveForPiece(9999).has_value());
}

TEST_CASE("activeMoveForPiece returns the in-flight move's exact from/to/timing for a moving piece")
{
    Board b = makeBoard({{"wR", ".", ".", "."}});
    RealTimeArbiter arbiter;
    const Piece *rook = b.pieceAt(Position{0, 0});
    REQUIRE(rook != nullptr);

    PieceMove m = makeMove(b, 0, 0, 0, 3, /*startMs*/ 100, /*durationMs*/ 900);
    arbiter.startMotion(b, m);

    const std::optional<PieceMove> found = arbiter.activeMoveForPiece(rook->id);
    REQUIRE(found.has_value());
    CHECK(found->fromRow == 0);
    CHECK(found->fromCol == 0);
    CHECK(found->toRow == 0);
    CHECK(found->toCol == 3);
    CHECK(found->startMs == 100);
    CHECK(found->durationMs == 900);
}

TEST_CASE("activeMoveForPiece stops returning a value once the move has resolved")
{
    Board b = makeBoard({{"wR", ".", "."}});
    RealTimeArbiter arbiter;
    const Piece *rook = b.pieceAt(Position{0, 0});
    REQUIRE(rook != nullptr);
    const int rookId = rook->id;

    PieceMove m = makeMove(b, 0, 0, 0, 2, 0, 500);
    arbiter.startMotion(b, m);
    REQUIRE(arbiter.activeMoveForPiece(rookId).has_value());

    arbiter.resolveMoves(b, 600, registry);

    CHECK_FALSE(arbiter.activeMoveForPiece(rookId).has_value());
}

TEST_CASE("activeMoveForPiece does not confuse two different pieces' in-flight moves")
{
    Board b = makeBoard({{"wR", ".", ".", "wB"}});
    RealTimeArbiter arbiter;
    const Piece *rook = b.pieceAt(Position{0, 0});
    const Piece *bishop = b.pieceAt(Position{0, 3});
    REQUIRE(rook != nullptr);
    REQUIRE(bishop != nullptr);

    arbiter.startMotion(b, makeMove(b, 0, 0, 0, 1, 0, 500));
    arbiter.startMotion(b, makeMove(b, 0, 3, 0, 2, 0, 700));

    const auto rookMove = arbiter.activeMoveForPiece(rook->id);
    const auto bishopMove = arbiter.activeMoveForPiece(bishop->id);

    REQUIRE(rookMove.has_value());
    REQUIRE(bishopMove.has_value());
    CHECK(rookMove->toCol == 1);
    CHECK(rookMove->durationMs == 500);
    CHECK(bishopMove->toCol == 2);
    CHECK(bishopMove->durationMs == 700);
}

TEST_CASE("resolveMoves puts a piece into RestingLong after a move lands")
{
    Board b = makeBoard({{"wR", ".", "."}});
    RealTimeArbiter arbiter;
    arbiter.startMotion(b, makeMove(b, 0, 0, 0, 2, 0, 500));

    arbiter.resolveMoves(b, 500, registry);

    const Piece *rook = b.pieceAt(Position{0, 2});
    REQUIRE(rook != nullptr);
    CHECK(rook->state == PieceState::RestingLong);
}

TEST_CASE("a piece in RestingLong returns to Idle once its rest window elapses")
{
    Board b = makeBoard({{"wR", ".", "."}});
    RealTimeArbiter arbiter;
    arbiter.startMotion(b, makeMove(b, 0, 0, 0, 2, 0, 500));
    arbiter.resolveMoves(b, 500, registry); // lands -> RestingLong

    arbiter.resolveMoves(b, 500 + config::statsFor('R').longRestMs, registry);

    const Piece *rook = b.pieceAt(Position{0, 2});
    REQUIRE(rook != nullptr);
    CHECK(rook->state == PieceState::Idle);
}

TEST_CASE("startJump sets the piece's state to Jumping, not Moving")
{
    Board b = makeBoard({{"wR", "."}});
    RealTimeArbiter arbiter;
    arbiter.startJump(b, makeJump(b, 0, 0, 0, 1000));

    const Piece *rook = b.pieceAt(Position{0, 0});
    REQUIRE(rook != nullptr);
    CHECK(rook->state == PieceState::Jumping);
}

TEST_CASE("a landed jump puts the piece into RestingShort")
{
    Board b = makeBoard({{"bR"}});
    RealTimeArbiter arbiter;
    arbiter.startJump(b, makeJump(b, 0, 0, 0, 1000));

    arbiter.resolveMoves(b, 1000, registry);

    const Piece *rook = b.pieceAt(Position{0, 0});
    REQUIRE(rook != nullptr);
    CHECK(rook->state == PieceState::RestingShort);
}

// --- Mid-flight collisions: two concurrently in-flight straight-line
// moves whose paths cross the same cell at (near-)the same instant, well
// before either one reaches its own final destination. ---

TEST_CASE("different-color pieces crossing mid-path: the later arrival eats the earlier one")
{
    // wR: (0,2) -> (7,2) vertical. bR: (4,0) -> (4,7) horizontal.
    // Their paths cross at (4,2) - neither piece's own destination.
    Board b = makeBoard({{".", ".", "wR", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {"bR", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."}});
    RealTimeArbiter arbiter;

    // wR reaches (4,2) - the 4th path cell of 7 - at 700*4/7 = 400ms.
    arbiter.startMotion(b, makeMove(b, 0, 2, 7, 2, 0, 700));
    // bR reaches (4,2) - the 2nd path cell of 7 - at 220 + 700*2/7 = 420ms.
    arbiter.startMotion(b, makeMove(b, 4, 0, 4, 7, 220, 700));

    arbiter.resolveMoves(b, 400, registry);

    CHECK(tokenAt(b, 0, 2) == "."); // wR (earlier arrival) captured, never lands
    CHECK_FALSE(arbiter.isPieceInFlight(0, 2));
    CHECK(arbiter.isPieceInFlight(4, 0)); // bR (later arrival) keeps flying to (4,7)
}

TEST_CASE("same-color pieces almost colliding mid-path: the later mover stops one cell short, along its own path")
{
    // The exact scenario from the assignment: a rook and a queen of the
    // same color would cross paths mid-flight; the later one stops one
    // cell before the crossing point on ITS OWN path, not the other
    // piece's path.
    Board b = makeBoard({{".", ".", "wR", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {"wQ", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."}});
    RealTimeArbiter arbiter;

    arbiter.startMotion(b, makeMove(b, 0, 2, 7, 2, 0, 700));   // wR: reaches (4,2) at 400ms
    arbiter.startMotion(b, makeMove(b, 4, 0, 4, 7, 220, 700)); // wQ: reaches (4,2) at 420ms - later

    arbiter.resolveMoves(b, 400, registry);

    // wQ (later) stops at (4,1) - one cell short of (4,2) along its own
    // (horizontal) path - and, since enough time has already passed for
    // that shorter hop, lands there within this same call.
    CHECK(tokenAt(b, 4, 1) == "wQ");
    CHECK_FALSE(arbiter.isPieceInFlight(4, 0));

    // wR (earlier arrival) is completely untouched - still flying toward (7,2).
    CHECK(arbiter.isPieceInFlight(0, 2));
}

TEST_CASE("same-line, opposite-direction same-color moves: only the single true meeting cell counts as a collision")
{
    // wR: (0,3) -> (7,3) downward. wQ: (6,3) -> (2,3) upward - same file,
    // paths overlap over rows 2-6 (4 shared cells), but only ONE of those
    // cell/time pairs is actually within tolerance: row 5, where the rook
    // arrives at 500ms and the queen at 460ms (40ms apart). Every other
    // shared row (4, 3, 2) is hundreds of ms apart and must NOT trigger.
    Board b = makeBoard({{".", ".", ".", "wR"},
                          {".", ".", ".", "."},
                          {".", ".", ".", "."},
                          {".", ".", ".", "."},
                          {".", ".", ".", "."},
                          {".", ".", ".", "."},
                          {".", ".", ".", "wQ"},
                          {".", ".", ".", "."}});
    RealTimeArbiter arbiter;

    arbiter.startMotion(b, makeMove(b, 0, 3, 7, 3, 0, 700));   // wR: row 5 at 500ms
    arbiter.startMotion(b, makeMove(b, 6, 3, 2, 3, 360, 400)); // wQ: row 5 at 460ms

    arbiter.resolveMoves(b, 500, registry);

    // wR (later at row 5, by 40ms) stops one cell short: row 4 - not row 6
    // or anywhere else a naive "first shared cell found" scan might pick.
    CHECK(tokenAt(b, 4, 3) == "wR");
    CHECK_FALSE(arbiter.isPieceInFlight(0, 3));

    // wQ (earlier at row 5) is completely unaffected - keeps flying toward row 2.
    CHECK(arbiter.isPieceInFlight(6, 3));
}

TEST_CASE("a knight crossing another piece's path mid-flight is unaffected - landing-only behavior is preserved")
{
    // Knights have no MoveRule::pathCells (non-sliding), so they are never
    // compared for mid-flight collisions at all, regardless of any
    // incidental cell overlap with another active move.
    Board b = makeBoard({{"wN", ".", ".", "."},
                          {"bR", ".", ".", "."},
                          {".", ".", ".", "."}});
    RealTimeArbiter arbiter;

    arbiter.startMotion(b, makeMove(b, 0, 0, 2, 1, 0, 300)); // wN: (0,0) -> (2,1)
    arbiter.startMotion(b, makeMove(b, 1, 0, 1, 3, 0, 300)); // bR: (1,0) -> (1,3)

    std::vector<Piece> captured = arbiter.resolveMoves(b, 300, registry);

    CHECK(captured.empty());
    CHECK(tokenAt(b, 2, 1) == "wN");
    CHECK(tokenAt(b, 1, 3) == "bR");
}

// --- Same "friendly can't reach a taken destination" scenario, two
// different code paths depending on timing - both are intentional, this
// pair of tests locks in and contrasts both. ---

TEST_CASE("two same-color movers still in flight, converging on the same destination: the later one stops one cell short")
{
    // wR: (0,0) -> (0,2). wQ: (2,2) -> (0,2). Both still active when
    // resolveMoves runs - resolveMidFlightCollisions catches this (their
    // paths' last cell IS the shared destination) and truncates the later
    // mover, same as any other mid-path collision. Contrast with the test
    // below, where the blocker has already landed.
    Board b = makeBoard({{"wR", ".", "."},
                          {".", ".", "."},
                          {".", ".", "wQ"}});
    RealTimeArbiter arbiter;

    arbiter.startMotion(b, makeMove(b, 0, 0, 0, 2, 0, 200));  // wR reaches (0,2) at 200ms
    arbiter.startMotion(b, makeMove(b, 2, 2, 0, 2, 20, 200)); // wQ reaches (0,2) at 220ms - later

    arbiter.resolveMoves(b, 220, registry);

    CHECK(tokenAt(b, 0, 2) == "wR"); // wR (earlier) lands normally
    CHECK(tokenAt(b, 1, 2) == "wQ"); // wQ (later) stops one cell short of (0,2)
    CHECK_FALSE(arbiter.isPieceInFlight(2, 2));
}

TEST_CASE("a same-color mover arriving after the blocker has already landed: the whole move fails, stays at its own origin")
{
    // Same conceptual scenario as above (a friendly piece is already at
    // the destination) but the blocker (wR) fully lands BEFORE wQ's move
    // is even started - so by the time wQ's own arrival is processed,
    // wR is no longer in activeMoves_ at all, and resolveMidFlightCollisions
    // never sees this pair. wQ instead falls through to the pre-existing
    // "friendly piece blocks destination" fallback in the ordinary
    // due-arrival loop: it stays completely at its own origin, not one
    // cell short - a materially different outcome for what looks like the
    // same situation, purely because of timing. Documented as intentional.
    Board b = makeBoard({{"wR", ".", "."},
                          {".", ".", "."},
                          {".", ".", "."}});
    RealTimeArbiter arbiter;

    arbiter.startMotion(b, makeMove(b, 0, 0, 0, 2, 0, 200));
    arbiter.resolveMoves(b, 200, registry); // wR fully lands at (0,2)

    Piece *blocker = b.pieceAt(Position{0, 2});
    REQUIRE(blocker != nullptr);
    b.addPiece('w', 'Q', Position{2, 2});
    arbiter.startMotion(b, makeMove(b, 2, 2, 0, 2, 200, 200)); // wQ: reaches (0,2) at 400ms

    arbiter.resolveMoves(b, 400, registry);

    CHECK(tokenAt(b, 0, 2) == "wR");     // blocker untouched
    CHECK(tokenAt(b, 2, 2) == "wQ");     // wQ never moved at all - stayed at its own origin
    CHECK_FALSE(arbiter.isPieceInFlight(2, 2));
}

// --- Exact ties (both color cases) and a 3-piece same-tick collision -
// documents the deterministic-but-order-dependent handling explicitly. ---

TEST_CASE("different-color pieces reaching the shared cell at the exact same instant: mutual capture, not a no-op")
{
    // wR: (0,2) -> (7,2), reaches (4,2) at exactly 400ms.
    // bR: (4,0) -> (4,7), reaches (4,2) at exactly 400ms too - a true tie.
    Board b = makeBoard({{".", ".", "wR", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {"bR", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."}});
    RealTimeArbiter arbiter;

    arbiter.startMotion(b, makeMove(b, 0, 2, 7, 2, 0, 700));   // wR reaches (4,2) at 700*4/7 = 400ms
    arbiter.startMotion(b, makeMove(b, 4, 0, 4, 7, 200, 700)); // bR reaches (4,2) at 200+700*2/7 = 400ms

    std::vector<Piece> captured = arbiter.resolveMoves(b, 400, registry);

    CHECK(captured.size() == 2);
    CHECK(tokenAt(b, 0, 2) == "."); // wR gone - never even started to land
    CHECK(tokenAt(b, 4, 0) == "."); // bR gone too
    CHECK_FALSE(arbiter.isPieceInFlight(0, 2));
    CHECK_FALSE(arbiter.isPieceInFlight(4, 0));
}

TEST_CASE("same-color pieces reaching the shared cell at the exact same instant: both stop one cell short")
{
    // Identical timing to the test above, same color this time.
    Board b = makeBoard({{".", ".", "wR", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {"wQ", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."}});
    RealTimeArbiter arbiter;

    arbiter.startMotion(b, makeMove(b, 0, 2, 7, 2, 0, 700));   // wR reaches (4,2) at 400ms
    arbiter.startMotion(b, makeMove(b, 4, 0, 4, 7, 200, 700)); // wQ reaches (4,2) at 400ms too

    arbiter.resolveMoves(b, 400, registry);

    CHECK(tokenAt(b, 3, 2) == "wR"); // one cell short of (4,2) along wR's own (vertical) path
    CHECK(tokenAt(b, 4, 1) == "wQ"); // one cell short of (4,2) along wQ's own (horizontal) path
    CHECK_FALSE(arbiter.isPieceInFlight(0, 2));
    CHECK_FALSE(arbiter.isPieceInFlight(4, 0));
}

TEST_CASE("three pieces colliding in the same tick: only the first pair (activeMoves_ order) is resolved, the third is left for a later tick")
{
    // A (wR, col 2) and C (bR, row 4) collide at (4,2): C is earlier (200ms)
    // than A (220ms) - different colors, so C is captured, A survives.
    // B (wR, col 5) and C would ALSO collide at (4,5) - an exact tie
    // (500ms each), which alone would be a dramatic mutual capture - but C
    // is already in `resolved` from the A/C pair by the time the B/C pair
    // is checked, so that pair is skipped outright. This is the intentional,
    // deterministic (loop-order-dependent, not "most correct") behavior the
    // `resolved` bookkeeping exists for.
    Board b = makeBoard({{".", ".", "wR", ".", ".", "wR", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {"bR", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."},
                          {".", ".", ".", ".", ".", ".", ".", "."}});
    RealTimeArbiter arbiter;

    arbiter.startMotion(b, makeMove(b, 0, 2, 7, 2, 180, 70));  // A: reaches (4,2) at 180+70*4/7 = 220ms
    arbiter.startMotion(b, makeMove(b, 0, 5, 7, 5, 100, 700)); // B: reaches (4,5) at 100+700*4/7 = 500ms
    arbiter.startMotion(b, makeMove(b, 4, 0, 4, 7, 0, 700));   // C: reaches (4,2) at 200ms, (4,5) at 500ms

    std::vector<Piece> captured = arbiter.resolveMoves(b, 500, registry);

    REQUIRE(captured.size() == 1);
    CHECK(captured[0].kind == 'R');
    CHECK(tokenAt(b, 4, 0) == "."); // C captured via the A/C pair

    CHECK(tokenAt(b, 7, 2) == "wR"); // A (the winner against C) reached its own destination normally
    CHECK(arbiter.isPieceInFlight(0, 5)); // B is completely untouched - its pairing with C never even ran
}