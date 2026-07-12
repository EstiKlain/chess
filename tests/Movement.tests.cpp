#include "doctest.h"

#include "Movement.hpp"
#include "Board.hpp"
#include "GameState.hpp"

namespace {
    PieceMove makeMove(int fromRow, int fromCol, int toRow, int toCol, const std::string& piece) {
        PieceMove m;
        m.fromRow = fromRow; m.fromCol = fromCol;
        m.toRow = toRow;     m.toCol = toCol;
        m.startMs = 0;       m.durationMs = 0;
        m.piece = piece;
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
    CHECK(isLegalMove(b, makeMove(0, 0, 1, 1, "wK"), 'K').isValid);
    CHECK(isLegalMove(b, makeMove(0, 0, 0, 1, "wK"), 'K').isValid);
    CHECK_FALSE(isLegalMove(b, makeMove(0, 0, 2, 2, "wK"), 'K').isValid);
    CHECK_FALSE(isLegalMove(b, makeMove(0, 0, 0, 0, "wK"), 'K').isValid);
}

TEST_CASE("isLegalMove: rook moves straight and needs a clear path") {
    Board clear = parseBoard({"wR . . .", ". . . .", ". . . .", ". . . ."});
    CHECK(isLegalMove(clear, makeMove(0, 0, 0, 3, "wR"), 'R').isValid);
    CHECK_FALSE(isLegalMove(clear, makeMove(0, 0, 1, 1, "wR"), 'R').isValid);

    Board blocked = parseBoard({"wR wP . ."});
    CHECK_FALSE(isLegalMove(blocked, makeMove(0, 0, 0, 3, "wR"), 'R').isValid);
}

TEST_CASE("isLegalMove: bishop moves diagonally and needs a clear path") {
    Board clear = parseBoard({
        "wB . . .",
        ". . . .",
        ". . . .",
        ". . . ."
    });
    CHECK(isLegalMove(clear, makeMove(0, 0, 3, 3, "wB"), 'B').isValid);
    CHECK_FALSE(isLegalMove(clear, makeMove(0, 0, 3, 2, "wB"), 'B').isValid);

    Board blocked = parseBoard({
        "wB . . .",
        ". wP . .",
        ". . . .",
        ". . . ."
    });
    CHECK_FALSE(isLegalMove(blocked, makeMove(0, 0, 2, 2, "wB"), 'B').isValid);
}

TEST_CASE("isLegalMove: queen moves like rook or bishop but not like a knight") {
    Board b = parseBoard({
        "wQ . . .",
        ". . . .",
        ". . . .",
        ". . . ."
    });
    CHECK(isLegalMove(b, makeMove(0, 0, 0, 3, "wQ"), 'Q').isValid);
    CHECK(isLegalMove(b, makeMove(0, 0, 3, 3, "wQ"), 'Q').isValid);
    CHECK_FALSE(isLegalMove(b, makeMove(0, 0, 1, 2, "wQ"), 'Q').isValid);
}

TEST_CASE("isLegalMove: knight moves in an L shape and ignores blockers") {
    Board b = parseBoard({
        "wN wP . .",
        "wP wP . .",
        ". . . .",
        ". . . ."
    });
    CHECK(isLegalMove(b, makeMove(0, 0, 2, 1, "wN"), 'N').isValid);
    CHECK_FALSE(isLegalMove(b, makeMove(0, 0, 1, 1, "wN"), 'N').isValid);
}

TEST_CASE("isLegalMove: pawn advances straight only onto an empty square") {
    Board b = parseBoard({
        ". . .",
        "wP . bP",
        ". . ."
    });
    CHECK(isLegalMove(b, makeMove(1, 0, 0, 0, "wP"), 'P').isValid);
    CHECK_FALSE(isLegalMove(b, makeMove(1, 0, 0, 1, "wP"), 'P').isValid);
    CHECK_FALSE(isLegalMove(b, makeMove(1, 2, 0, 2, "bP"), 'P').isValid);
}

TEST_CASE("isLegalMove: pawn captures diagonally only, never straight") {
    Board b = parseBoard({
        "bP . bP",
        ". wP .",
        ". . ."
    });
    CHECK(isLegalMove(b, makeMove(1, 1, 0, 0, "wP"), 'P').isValid);
    CHECK(isLegalMove(b, makeMove(1, 1, 0, 2, "wP"), 'P').isValid);

    Board straightIntoEnemy = parseBoard({"bP", "wP"});
    CHECK_FALSE(isLegalMove(straightIntoEnemy, makeMove(1, 0, 0, 0, "wP"), 'P').isValid);
}

TEST_CASE("isLegalMove: a piece may never capture its own color") {
    Board b = parseBoard({"wR wP . ."});
    CHECK_FALSE(isLegalMove(b, makeMove(0, 0, 0, 1, "wR"), 'R').isValid);
}

TEST_CASE("isLegalMove: pieces with no registered shape are unrestricted") {
    Board b = parseBoard({"wX . . .", ". . . .", ". . . .", ". . . ."});
    CHECK(isLegalMove(b, makeMove(0, 0, 3, 1, "wX"), 'X').isValid);
}

TEST_CASE("isLegalMove reasons") {
    SUBCASE("friendly_destination") {
        Board b = parseBoard({"wR wP . ."});
        auto res = isLegalMove(b, makeMove(0, 0, 0, 1, "wR"), 'R');
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "friendly_destination");
    }

    SUBCASE("illegal_piece_move") {
        Board b = parseBoard({"wR . . .", ". . . ."});
        auto res = isLegalMove(b, makeMove(0, 0, 1, 1, "wR"), 'R');
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "illegal_piece_move");
    }

    SUBCASE("blocked_path") {
        Board b = parseBoard({"wR wP . ."});
        auto res = isLegalMove(b, makeMove(0, 0, 0, 2, "wR"), 'R');
        CHECK_FALSE(res.isValid);
        CHECK(res.reason == "blocked_path");
    }

    SUBCASE("legal") {
        Board b = parseBoard({"wR . . .", ". . . ."});
        auto res = isLegalMove(b, makeMove(0, 0, 0, 3, "wR"), 'R');
        CHECK(res.isValid);
        CHECK(res.reason == "");
    }
}
