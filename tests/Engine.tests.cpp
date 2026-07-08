#include "doctest.h"

#include "Engine.hpp"
#include "Board.hpp"
#include "GameState.hpp"

#include <sstream>
#include <iostream>

namespace {
    GameState makeState(const std::vector<std::string>& boardLines) {
        GameState st;
        st.board = parseBoard(boardLines);
        st.selections.assign(2, Selection{});
        return st;
    }
}

TEST_CASE("resolveMoves keeps a move active before its arrival time") {
    GameState st = makeState({"wR . . ."});
    PieceMove m; m.fromRow = 0; m.fromCol = 0; m.toRow = 0; m.toCol = 3;
    m.startMs = 0; m.durationMs = 1000; m.piece = "wR";
    st.activeMoves.push_back(m);
    st.elapsedMs = 500;

    resolveMoves(st);

    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.board.grid[0][3] == ".");
}

TEST_CASE("resolveMoves lands a move onto an empty target once due") {
    GameState st = makeState({"wR . . ."});
    PieceMove m; m.fromRow = 0; m.fromCol = 0; m.toRow = 0; m.toCol = 3;
    m.startMs = 0; m.durationMs = 1000; m.piece = "wR";
    st.activeMoves.push_back(m);
    st.elapsedMs = 1000;

    resolveMoves(st);

    CHECK(st.activeMoves.empty());
    CHECK(st.board.grid[0][3] == "wR");
}

TEST_CASE("resolveMoves captures an enemy occupying the target") {
    GameState st = makeState({"wR . . bP"});
    PieceMove m; m.fromRow = 0; m.fromCol = 0; m.toRow = 0; m.toCol = 3;
    m.startMs = 0; m.durationMs = 500; m.piece = "wR";
    st.activeMoves.push_back(m);
    st.elapsedMs = 500;

    resolveMoves(st);

    CHECK(st.board.grid[0][3] == "wR");
}

TEST_CASE("resolveMoves bounces a piece home when the target turned friendly and the source is free") {
    GameState st = makeState({". . . wP"});
    PieceMove m; m.fromRow = 0; m.fromCol = 0; m.toRow = 0; m.toCol = 3;
    m.startMs = 0; m.durationMs = 500; m.piece = "wR";
    st.activeMoves.push_back(m);
    st.elapsedMs = 500;

    resolveMoves(st);

    CHECK(st.board.grid[0][0] == "wR");
    CHECK(st.board.grid[0][3] == "wP");
}

TEST_CASE("resolveMoves loses a piece when the target turned friendly and the source is occupied") {
    GameState st = makeState({"wN . . wP"});
    PieceMove m; m.fromRow = 0; m.fromCol = 0; m.toRow = 0; m.toCol = 3;
    m.startMs = 0; m.durationMs = 500; m.piece = "wR";
    st.activeMoves.push_back(m);
    st.elapsedMs = 500;

    resolveMoves(st);

    CHECK(st.board.grid[0][0] == "wN");
    CHECK(st.board.grid[0][3] == "wP");
}

TEST_CASE("resolveMoves settles multiple due moves in arrival order") {
    GameState st = makeState({". . ."});
    PieceMove first; first.fromRow = 0; first.fromCol = 0; first.toRow = 0; first.toCol = 1;
    first.startMs = 0; first.durationMs = 200; first.piece = "wP";
    PieceMove second; second.fromRow = 0; second.fromCol = 2; second.toRow = 0; second.toCol = 1;
    second.startMs = 0; second.durationMs = 100; second.piece = "bP";
    st.activeMoves = {first, second};
    st.elapsedMs = 300;

    resolveMoves(st);

    CHECK(st.board.grid[0][1] == "wP");
}

TEST_CASE("sendMove clears the source and queues an active move for a legal move") {
    GameState st = makeState({"wR . . .", ". . . .", ". . . .", ". . . ."});
    st.selections[0] = {true, 0, 0, 0};

    sendMove(st, 0, 0, 3);

    CHECK(st.board.grid[0][0] == ".");
    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.activeMoves[0].piece == "wR");
    CHECK(st.activeMoves[0].toCol == 3);
    CHECK_FALSE(st.selections[0].active);
}

TEST_CASE("sendMove ignores an illegal move and leaves the board untouched") {
    GameState st = makeState({"wR . . .", ". . . .", ". . . .", ". . . ."});
    st.selections[0] = {true, 0, 0, 0};

    sendMove(st, 0, 1, 1); // diagonal - illegal for a rook

    CHECK(st.board.grid[0][0] == "wR");
    CHECK(st.activeMoves.empty());
    CHECK_FALSE(st.selections[0].active);
}

TEST_CASE("sendMove computes duration from piece speed and travel distance") {
    GameState st = makeState({"wQ . . .", ". . . .", ". . . .", ". . . ."});
    st.selections[0] = {true, 0, 0, 0};

    sendMove(st, 0, 0, 3); // queen: 4 cells/sec, 3 cells travelled

    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.activeMoves[0].durationMs == 750);
}

TEST_CASE("handleClick opens a fresh selection when the claimed player clicks their own idle piece") {
    GameState st = makeState({"wK . . .", ". . . .", ". . . .", ". . . ."});

    handleClick(st, 0, 5, 5); // player 0 (white), inside cell (0,0)

    CHECK(st.selections[0].active);
    CHECK(st.selections[0].row == 0);
    CHECK(st.selections[0].col == 0);
}

TEST_CASE("handleClick rejects a claimed player that does not own the clicked piece") {
    GameState st = makeState({"wK . . .", ". . . .", ". . . .", ". . . ."});

    handleClick(st, 1, 5, 5); // player 1 (black) claims a white piece

    CHECK_FALSE(st.selections[1].active);
}

TEST_CASE("handleClick reselects when the pending player clicks another piece of their own") {
    GameState st = makeState({"wK . wQ .", ". . . .", ". . . .", ". . . ."});
    st.selections[0] = {true, 0, 0, 0};

    handleClick(st, 0, 205, 5); // player 0 clicks cell (0,2), also white

    CHECK(st.selections[0].active);
    CHECK(st.selections[0].col == 2);
}

TEST_CASE("handleClick completes the player's own pending selection when clicking an empty cell") {
    GameState st = makeState({"wR . . .", ". . . .", ". . . .", ". . . ."});
    st.selections[0] = {true, 0, 0, 0};

    handleClick(st, 0, 305, 5); // player 0 clicks empty cell (0,3)

    CHECK(st.board.grid[0][0] == ".");
    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.activeMoves[0].toCol == 3);
}

TEST_CASE("handleClick completes the player's own pending selection as a capture on an enemy cell") {
    GameState st = makeState({"wR . . bP", ". . . .", ". . . .", ". . . ."});
    st.selections[0] = {true, 0, 0, 0};

    handleClick(st, 0, 305, 5); // player 0 clicks the black pawn at (0,3)

    CHECK(st.board.grid[0][0] == ".");
    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.activeMoves[0].toCol == 3);
    CHECK_FALSE(st.selections[1].active);
}

TEST_CASE("handleClick does nothing when a player with no pending selection clicks the opponent's piece") {
    GameState st = makeState({"wR . . bP", ". . . .", ". . . .", ". . . ."});

    handleClick(st, 0, 305, 5); // player 0 clicks black's pawn, nobody is pending

    CHECK(st.board.grid[0][3] == "bP");
    CHECK(st.activeMoves.empty());
    CHECK_FALSE(st.selections[0].active);
    CHECK_FALSE(st.selections[1].active);
}

TEST_CASE("handleClick ignores clicks outside the board") {
    GameState st = makeState({"wK ."});
    handleClick(st, 0, -5, -5);
    handleClick(st, 0, 10000, 10000);

    CHECK_FALSE(st.selections[0].active);
    CHECK_FALSE(st.selections[1].active);
}

TEST_CASE("handleClick ignores an out-of-range player index") {
    GameState st = makeState({"wK ."});
    handleClick(st, 5, 5, 5);
    handleClick(st, -1, 5, 5);
    CHECK_FALSE(st.selections[0].active);
    CHECK_FALSE(st.selections[1].active);
}

TEST_CASE("handleClick on an empty cell with no pending selection is a no-op") {
    GameState st = makeState({". . .", ". . .", ". . ."});
    handleClick(st, 0, 5, 5);
    CHECK_FALSE(st.selections[0].active);
    CHECK_FALSE(st.selections[1].active);
}

TEST_CASE("handleWait advances the clock and resolves due moves") {
    GameState st = makeState({"wR . . ."});
    PieceMove m; m.fromRow = 0; m.fromCol = 0; m.toRow = 0; m.toCol = 3;
    m.startMs = 0; m.durationMs = 100; m.piece = "wR";
    st.activeMoves.push_back(m);

    handleWait(st, 150);

    CHECK(st.elapsedMs == 150);
    CHECK(st.activeMoves.empty());
    CHECK(st.board.grid[0][3] == "wR");
}

TEST_CASE("runCommands executes click, wait and print in sequence") {
    GameState st = makeState({"wR . . .", ". . . .", ". . . .", ". . . ."});

    std::ostringstream captured;
    std::streambuf* old = std::cout.rdbuf(captured.rdbuf());

    std::vector<std::string> commands = {
        "click 0 5 5",
        "click 0 305 5",
        "wait 1000",
        "print board"
    };
    runCommands(commands, st);

    std::cout.rdbuf(old);

    CHECK(captured.str() == formatBoard(st.board));
    CHECK(st.board.grid[0][3] == "wR");
}
