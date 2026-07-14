#include "doctest.h"

#include "Movement.hpp"
#include "model/Board.hpp"
#include "BoardParser.hpp"

static pieceRules::PieceRulesRegistry registry;
namespace {
    Board parseBoard(const std::vector<std::string>& lines) {
        RawBoard raw = parseRawGrid(lines);
        return buildBoard(raw);
    }

    // Looks up whatever piece actually sits at (fromRow, fromCol) on the
    // given board and uses its id - this is what checkPieceShape now
    // needs to resolve the mover's color, replacing the old inline
    // "wK"-style token that used to travel inside PieceMove itself.
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

TEST_CASE("playerIndexOf maps colors to indices") {
    CHECK(playerIndexOf('w') == 0);
    CHECK(playerIndexOf('b') == 1);
    CHECK(playerIndexOf('x') == -1);
}

TEST_CASE("cellDistance computes euclidean distance in cells") {
    CHECK(cellDistance(0, 0, 3, 4) == doctest::Approx(5.0));
    CHECK(cellDistance(2, 2, 2, 2) == doctest::Approx(0.0));
    CHECK(cellDistance(0, 0, 0, 5) == doctest::Approx(5.0));
}

TEST_CASE("isLegalMove: king moves one square in any direction") {
    Board b = parseBoard({"wK . .", ". . .", ". . ."});
    CHECK(checkPieceShape(b, makeMove(b, 0, 0, 1, 1), 'K', registry).isValid);
    CHECK(checkPieceShape(b, makeMove(b, 0, 0, 0, 1), 'K', registry).isValid);
    CHECK_FALSE(checkPieceShape(b, makeMove(b, 0, 0, 2, 2), 'K', registry).isValid);
    CHECK_FALSE(checkPieceShape(b, makeMove(b, 0, 0, 0, 0), 'K', registry).isValid);
}

TEST_CASE("isLegalMove: rook moves straight and needs a clear path") {
    Board clear = parseBoard({"wR . . .", ". . . .", ". . . .", ". . . ."});
    CHECK(checkPieceShape(clear, makeMove(clear, 0, 0, 0, 3), 'R', registry).isValid);
    CHECK_FALSE(checkPieceShape(clear, makeMove(clear, 0, 0, 1, 1), 'R', registry).isValid);

    Board blocked = parseBoard({"wR wP . ."});
    CHECK_FALSE(checkPieceShape(blocked, makeMove(blocked, 0, 0, 0, 3), 'R', registry).isValid);
}

TEST_CASE("isLegalMove: bishop moves diagonally and needs a clear path") {
    Board clear = parseBoard({
        "wB . . .",
        ". . . .",
        ". . . .",
        ". . . ."
    });
    CHECK(checkPieceShape(clear, makeMove(clear, 0, 0, 3, 3), 'B', registry).isValid);
    CHECK_FALSE(checkPieceShape(clear, makeMove(clear, 0, 0, 3, 2), 'B', registry).isValid);

    Board blocked = parseBoard({
        "wB . . .",
        ". wP . .",
        ". . . .",
        ". . . ."
    });
    CHECK_FALSE(checkPieceShape(blocked, makeMove(blocked, 0, 0, 2, 2), 'B', registry).isValid);
}

TEST_CASE("isLegalMove: queen moves like rook or bishop but not like a knight") {
    Board b = parseBoard({
        "wQ . . .",
        ". . . .",
        ". . . .",
        ". . . ."
    });
    CHECK(checkPieceShape(b, makeMove(b, 0, 0, 0, 3), 'Q', registry).isValid);
    CHECK(checkPieceShape(b, makeMove(b, 0, 0, 3, 3), 'Q', registry).isValid);
    CHECK_FALSE(checkPieceShape(b, makeMove(b, 0, 0, 1, 2), 'Q', registry).isValid);
}

TEST_CASE("isLegalMove: knight moves in an L shape and ignores blockers") {
    Board b = parseBoard({
        "wN wP . .",
        "wP wP . .",
        ". . . .",
        ". . . ."
    });
    CHECK(checkPieceShape(b, makeMove(b, 0, 0, 2, 1), 'N', registry).isValid);
    CHECK_FALSE(checkPieceShape(b, makeMove(b, 0, 0, 1, 1), 'N', registry).isValid);
}

TEST_CASE("isLegalMove: pawn advances straight only onto an empty square") {
    Board b = parseBoard({
        ". . .",
        "wP . bP",
        ". . ."
    });
    CHECK(checkPieceShape(b, makeMove(b, 1, 0, 0, 0), 'P', registry).isValid);
    CHECK_FALSE(checkPieceShape(b, makeMove(b, 1, 0, 0, 1), 'P', registry).isValid);
    CHECK_FALSE(checkPieceShape(b, makeMove(b, 1, 2, 0, 2), 'P', registry).isValid);
}

TEST_CASE("isLegalMove: pawn captures diagonally only, never straight") {
    Board b = parseBoard({
        "bP . bP",
        ". wP .",
        ". . ."
    });
    CHECK(checkPieceShape(b, makeMove(b, 1, 1, 0, 0), 'P', registry).isValid);
    CHECK(checkPieceShape(b, makeMove(b, 1, 1, 0, 2), 'P', registry).isValid);

    Board straightIntoEnemy = parseBoard({"bP", "wP"});
    CHECK_FALSE(checkPieceShape(straightIntoEnemy, makeMove(straightIntoEnemy, 1, 0, 0, 0), 'P', registry).isValid);
}

TEST_CASE("isLegalMove: a piece may never capture its own color") {
    Board b = parseBoard({"wR wP . ."});
    CHECK_FALSE(checkPieceShape(b, makeMove(b, 0, 0, 0, 1), 'R', registry).isValid);
}

TEST_CASE("isLegalMove: pieces with no registered shape are unrestricted") {
    Board b = parseBoard({"wX . . .", ". . . .", ". . . .", ". . . ."});
    CHECK(checkPieceShape(b, makeMove(b, 0, 0, 3, 1), 'X', registry).isValid);
}

TEST_CASE("isLegalMove reasons") {
    SUBCASE("friendly_destination") {
        Board b = parseBoard({"wR wP . ."});
        auto res = checkPieceShape(b, makeMove(b, 0, 0, 0, 1), 'R', registry);
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "friendly_destination");
    }

    SUBCASE("illegal_piece_move") {
        Board b = parseBoard({"wR . . .", ". . . ."});
        auto res = checkPieceShape(b, makeMove(b, 0, 0, 1, 1), 'R', registry);
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "illegal_piece_move");
    }

    SUBCASE("blocked_path") {
        Board b = parseBoard({"wR wP . ."});
        auto res = checkPieceShape(b, makeMove(b, 0, 0, 0, 2), 'R', registry);
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "blocked_path");
    }

    SUBCASE("legal") {
        Board b = parseBoard({"wR . . .", ". . . ."});
        auto res = checkPieceShape(b, makeMove(b, 0, 0, 0, 3), 'R', registry);
        CHECK(res.isValid);
        CHECK(res.reason == "legal");
    }
}

TEST_CASE("pawn on start row with clear path may double-step forward") {
    Board b = parseBoard({
        ". . . .",
        ". . . .",
        ". . . .",
        ". . . .",
        ". . . .",
        ". . . .",
        "wP . . .",
        ". . . ."
    });
    CHECK(checkPieceShape(b, makeMove(b, 6, 0, 4, 0), 'P', registry).isValid);
}

TEST_CASE("pawn double-step blocked when intermediate cell is occupied") {
    Board b = parseBoard({
        ". . . .",
        ". . . .",
        ". . . .",
        ". . . .",
        ". . . .",
        "wR . . .",
        "wP . . .",
        ". . . ."
    });
    auto res = checkPieceShape(b, makeMove(b, 6, 0, 4, 0), 'P', registry);
    CHECK_FALSE(res.isValid);
    CHECK(res.reason == "pawn_double_step_blocked");
}

TEST_CASE("pawn double-step rejected when not on canonical start row") {
    Board b = parseBoard({
        ". . . .",
        ". . . .",
        ". . . .",
        ". . . .",
        ". . . .",
        "wP . . .",
        ". . . .",
        ". . . ."
    });
    auto res = checkPieceShape(b, makeMove(b, 5, 0, 3, 0), 'P', registry);
    CHECK_FALSE(res.isValid);
    CHECK(res.reason == "pawn_double_step_blocked");
}

TEST_CASE("pawn one-cell forward and diagonal capture still work with contextGate") {
    Board forward = parseBoard({
        ". . .",
        "wP . .",
        ". . ."
    });
    CHECK(checkPieceShape(forward, makeMove(forward, 1, 0, 0, 0), 'P', registry).isValid);

    Board capture = parseBoard({
        "bP . bP",
        ". wP .",
        ". . ."
    });
    CHECK(checkPieceShape(capture, makeMove(capture, 1, 1, 0, 0), 'P', registry).isValid);
    CHECK(checkPieceShape(capture, makeMove(capture, 1, 1, 0, 2), 'P', registry).isValid);
}

TEST_CASE("pawn two-cell move onto non-empty destination is rejected") {
    Board friendly = parseBoard({
        ". . . .",
        ". . . .",
        ". . . .",
        ". . . .",
        "wP . . .",
        ". . . .",
        "wP . . .",
        ". . . ."
    });
    CHECK_FALSE(checkPieceShape(friendly, makeMove(friendly, 6, 0, 4, 0), 'P', registry).isValid);

    Board enemy = parseBoard({
        ". . . .",
        ". . . .",
        ". . . .",
        ". . . .",
        "bP . . .",
        ". . . .",
        "wP . . .",
        ". . . ."
    });
    auto res = checkPieceShape(enemy, makeMove(enemy, 6, 0, 4, 0), 'P', registry);
    CHECK_FALSE(res.isValid);
    CHECK(res.reason == "illegal_piece_move");
}
