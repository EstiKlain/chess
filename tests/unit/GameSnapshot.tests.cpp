// GameEngine::snapshot() is the single read-only gate out of the engine
// for Iteration D. These tests check (a) it reflects board contents
// correctly, and (b) it is a genuine value-copy - mutating the engine
// afterwards must NOT change a snapshot already taken.
#include "doctest.h"

#include "engine/GameEngine.hpp"
#include "io/BoardParser.hpp"

namespace
{
    Board makeBoard(std::initializer_list<std::initializer_list<std::string>> rows)
    {
        RawBoard raw;
        for (const auto &row : rows)
            raw.push_back(std::vector<std::string>(row.begin(), row.end()));
        return buildBoard(raw);
    }

    const PieceSnapshot *findAt(const GameSnapshot &s, int row, int col)
    {
        for (const auto &p : s.pieces)
            if (p.row == row && p.col == col)
                return &p;
        return nullptr;
    }
}

TEST_CASE("snapshot reports rows/cols and gameOver=false for a fresh board")
{
    Board board = makeBoard({{"wK", "."},
                              {".", "bK"}});
    GameEngine engine(board, pieceRules::PieceRulesRegistry());

    const GameSnapshot s = engine.snapshot();

    CHECK(s.rows == 2);
    CHECK(s.cols == 2);
    CHECK(s.gameOver == false);
}

TEST_CASE("snapshot contains exactly one PieceSnapshot per piece on the board")
{
    Board board = makeBoard({{"wK", "."},
                              {".", "bQ"}});
    GameEngine engine(board, pieceRules::PieceRulesRegistry());

    const GameSnapshot s = engine.snapshot();

    REQUIRE(s.pieces.size() == 2);

    const PieceSnapshot *king = findAt(s, 0, 0);
    REQUIRE(king != nullptr);
    CHECK(king->color == 'w');
    CHECK(king->kind == 'K');

    const PieceSnapshot *queen = findAt(s, 1, 1);
    REQUIRE(queen != nullptr);
    CHECK(queen->color == 'b');
    CHECK(queen->kind == 'Q');
}

TEST_CASE("a snapshot already taken is unaffected by later engine mutation (real value copy)")
{
    Board board = makeBoard({{"wK", ".", "."},
                              {".", ".", "."},
                              {".", ".", "bK"}});
    GameEngine engine(board, pieceRules::PieceRulesRegistry());

    const GameSnapshot before = engine.snapshot();
    REQUIRE(before.pieces.size() == 2);

    // Move the white king; advance the clock enough for the motion to
    // resolve. This mutates engine's internal Board.
    engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 1}});
    engine.wait(5000);

    // The OLD snapshot must still show the king at its original cell -
    // it was never a reference into board_, so the mutation above cannot
    // reach it.
    const PieceSnapshot *oldKing = findAt(before, 0, 0);
    REQUIRE(oldKing != nullptr);
    CHECK(oldKing->kind == 'K');

    // A NEW snapshot, taken now, should show the move having happened.
    const GameSnapshot after = engine.snapshot();
    CHECK(findAt(after, 0, 0) == nullptr);
    CHECK(findAt(after, 0, 1) != nullptr);
}

// --- Iteration E: motion / nowMs ----------------------------------------
// GameSnapshot::nowMs and PieceSnapshot::motion are what let PieceAnimator
// interpolate a mid-glide pixel position without ever touching
// RealTimeArbiter/PieceMove directly. See GameSnapshot.hpp for why nowMs
// has to come from the engine's own clock (elapsedMs_), not wall time.

TEST_CASE("snapshot's nowMs reflects the engine's own elapsed clock, not zero by default")
{
    Board board = makeBoard({{"wK", "."}});
    GameEngine engine(board, pieceRules::PieceRulesRegistry());

    CHECK(engine.snapshot().nowMs == 0);

    engine.wait(250);
    CHECK(engine.snapshot().nowMs == 250);

    engine.wait(100);
    CHECK(engine.snapshot().nowMs == 350); // cumulative, not reset per wait() call
}

TEST_CASE("a piece with no move in flight has motion == nullopt")
{
    Board board = makeBoard({{"wK", "."}});
    GameEngine engine(board, pieceRules::PieceRulesRegistry());

    const GameSnapshot s = engine.snapshot();
    const PieceSnapshot *king = findAt(s, 0, 0);
    REQUIRE(king != nullptr);
    CHECK_FALSE(king->motion.has_value());
}

TEST_CASE("a piece mid-move has motion populated with the exact from/to/timing of the accepted move")
{
    // Rook: 1 cell/sec (config::statsFor('R')) - 2 cells -> 2000ms duration,
    // clean round numbers to assert on.
    Board board = makeBoard({{"wR", ".", "."}});
    GameEngine engine(board, pieceRules::PieceRulesRegistry());

    const MoveResult result = engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 2}});
    REQUIRE(result.accepted);

    const GameSnapshot s = engine.snapshot();
    const PieceSnapshot *rook = findAt(s, 0, 0); // still at origin - the move hasn't landed yet
    REQUIRE(rook != nullptr);
    REQUIRE(rook->motion.has_value());

    const MotionSnapshot &m = *rook->motion;
    CHECK(m.fromRow == 0);
    CHECK(m.fromCol == 0);
    CHECK(m.toRow == 0);
    CHECK(m.toCol == 2);
    CHECK(m.startMs == 0);
    CHECK(m.durationMs == 2000);
}

TEST_CASE("motion stays populated partway through a move, then clears once it lands")
{
    Board board = makeBoard({{"wR", ".", "."}});
    GameEngine engine(board, pieceRules::PieceRulesRegistry());

    REQUIRE(engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 2}}).accepted);

    engine.wait(500); // 500 of 2000ms elapsed - still in flight
    const GameSnapshot mid = engine.snapshot();
    const PieceSnapshot *movingRook = findAt(mid, 0, 0);
    REQUIRE(movingRook != nullptr);
    REQUIRE(movingRook->motion.has_value());
    CHECK(mid.nowMs == 500);

    engine.wait(1600); // cumulative 2100ms - past the 2000ms duration
    const GameSnapshot done = engine.snapshot();
    const PieceSnapshot *landedRook = findAt(done, 0, 2);
    REQUIRE(landedRook != nullptr);
    CHECK_FALSE(landedRook->motion.has_value());
    CHECK(findAt(done, 0, 0) == nullptr); // no longer at the origin cell
}