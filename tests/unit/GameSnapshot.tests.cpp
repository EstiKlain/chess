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
