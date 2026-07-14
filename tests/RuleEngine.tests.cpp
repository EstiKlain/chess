#include "doctest.h"

#include "RuleEngine.hpp"
#include "model/Board.hpp"
#include "BoardParser.hpp"

static pieceRules::PieceRulesRegistry registry;

namespace {
    Board parseBoard(const std::vector<std::string>& lines) {
        RawBoard raw = parseRawGrid(lines);
        return buildBoard(raw);
    }

    PieceMove makeMove(const Board& b, int fromRow, int fromCol, int toRow, int toCol) {
        PieceMove m;
        m.fromRow = fromRow; m.fromCol = fromCol;
        m.toRow = toRow;     m.toCol = toCol;
        m.startMs = 0;       m.durationMs = 0;
        const Piece* p = b.pieceAt(Position{fromRow, fromCol});
        m.pieceId = p ? p->id : -1;
        return m;
    }
}

TEST_CASE("isMoveLegal delegates legal rook moves")
{
    Board b = parseBoard({"wR . ."});

    CHECK(isMoveLegal(b, makeMove(b, 0, 0, 0, 2), 'R', registry).isValid);
}

TEST_CASE("isMoveLegal rejects empty source before shape checks")
{
    Board b = parseBoard({". . wR"});

    const auto res = isMoveLegal(b, makeMove(b, 0, 0, 0, 2), 'R', registry);
    CHECK_FALSE(res.isValid);
    CHECK(res.reason == "empty_source");
}

TEST_CASE("isMoveLegal rejects bishop-shaped rook move")
{
    Board b = parseBoard({"wR . .", ". . ."});

    CHECK_FALSE(isMoveLegal(b, makeMove(b, 0, 0, 1, 1), 'R', registry).isValid);
}

TEST_CASE("isMoveLegal bounds checking")
{
    Board b = parseBoard({"wK .", ". ."}); // 2x2 board

    SUBCASE("outside_board - toRow too high") {
        auto res = isMoveLegal(b, makeMove(b, 0, 0, 2, 0), 'K', registry);
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "outside_board");
    }

    SUBCASE("outside_board - toRow too low") {
        auto res = isMoveLegal(b, makeMove(b, 0, 0, -1, 0), 'K', registry);
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "outside_board");
    }

    SUBCASE("outside_board - toCol too high") {
        auto res = isMoveLegal(b, makeMove(b, 0, 0, 0, 2), 'K', registry);
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "outside_board");
    }

    SUBCASE("outside_board - toCol too low") {
        auto res = isMoveLegal(b, makeMove(b, 0, 0, 0, -1), 'K', registry);
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "outside_board");
    }

    SUBCASE("forwards Movement's reason when in-bounds but illegal") {
        // King diagonal is legal, so use a Rook shape check instead,
        // while still resolving the mover's color from the actual wK
        // sitting at (0,0) on this board.
        auto res = isMoveLegal(b, makeMove(b, 0, 0, 1, 1), 'R', registry);
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "illegal_piece_move");
    }
}
