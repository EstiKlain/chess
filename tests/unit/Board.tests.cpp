#include "doctest.h"

#include "model/Board.hpp"
#include "legacy/BoardParser.hpp"
#include "legacy/BoardPrinter.hpp"

namespace {
    // Local helper: mirrors the old convenience parseBoard(lines) API by
    // combining the two production steps (split into RawBoard, then build
    // a real Board of Pieces) - no validation, matching old semantics.
    Board parseBoard(const std::vector<std::string>& lines) {
        RawBoard raw = parseRawGrid(lines);
        return buildBoard(raw);
    }

    // Local helper: reads back a single cell as a "colorkind" token (or
    // ".") purely for test assertions - Board itself has no such concept.
    std::string tokenAt(const Board& b, int row, int col) {
        const Piece* p = b.pieceAt(Position{row, col});
        if (!p) return ".";
        return std::string(1, p->color) + std::string(1, p->kind);
    }
}

TEST_CASE("trim removes leading and trailing whitespace") {
    CHECK(trim("  hello  ") == "hello");
    CHECK(trim("\t\n hi \t") == "hi");
    CHECK(trim("no_spaces") == "no_spaces");
    CHECK(trim("") == "");
    CHECK(trim("   ") == "");
}

TEST_CASE("splitWords splits on whitespace and ignores extra spaces") {
    CHECK(splitWords("a b c") == std::vector<std::string>{"a", "b", "c"});
    CHECK(splitWords("   a    b  ") == std::vector<std::string>{"a", "b"});
    CHECK(splitWords("") == std::vector<std::string>{});
    CHECK(splitWords("single") == std::vector<std::string>{"single"});
}

TEST_CASE("parseSections splits Board and Commands blocks") {
    std::string text =
        "Board:\n"
        "wR wN\n"
        "bR bN\n"
        "Commands:\n"
        "click 10 10\n"
        "wait 100\n";

    Sections s = parseSections(text);
    REQUIRE(s.boardLines.size() == 2);
    CHECK(s.boardLines[0] == "wR wN");
    CHECK(s.boardLines[1] == "bR bN");
    REQUIRE(s.commandLines.size() == 2);
    CHECK(s.commandLines[0] == "click 10 10");
    CHECK(s.commandLines[1] == "wait 100");
}

TEST_CASE("parseSections tolerates leading whitespace before headers and blank lines") {
    std::string text =
        "   Board:\n"
        "wK .\n"
        "\n"
        "  Commands:\n"
        "\n"
        "wait 50\n";

    Sections s = parseSections(text);
    REQUIRE(s.boardLines.size() == 1);
    CHECK(s.boardLines[0] == "wK .");
    REQUIRE(s.commandLines.size() == 1);
    CHECK(s.commandLines[0] == "wait 50");
}

TEST_CASE("parseSections ignores lines before any header") {
    std::string text = "garbage\nBoard:\nwK .\n";
    Sections s = parseSections(text);
    REQUIRE(s.boardLines.size() == 1);
    CHECK(s.boardLines[0] == "wK .");
}

TEST_CASE("parseBoard builds a board of pieces from board lines") {
    Board b = parseBoard({"wR wN", "bR bN"});
    REQUIRE(b.rows() == 2);
    REQUIRE(b.cols() == 2);
    CHECK(tokenAt(b, 0, 0) == "wR");
    CHECK(tokenAt(b, 0, 1) == "wN");
    CHECK(tokenAt(b, 1, 0) == "bR");
    CHECK(tokenAt(b, 1, 1) == "bN");
}

TEST_CASE("Board::rows and Board::cols report grid dimensions") {
    Board empty;
    CHECK(empty.rows() == 0);
    CHECK(empty.cols() == 0);

    Board b = parseBoard({"wK bK ."});
    CHECK(b.rows() == 1);
    CHECK(b.cols() == 3);
}

TEST_CASE("isValidToken accepts empty cell and well formed pieces") {
    CHECK(isValidToken("."));
    CHECK(isValidToken("wK"));
    CHECK(isValidToken("bQ"));
    CHECK(isValidToken("wR"));
    CHECK(isValidToken("bB"));
    CHECK(isValidToken("wN"));
    CHECK(isValidToken("bP"));
}

TEST_CASE("isValidToken rejects bad color, bad piece or bad length") {
    CHECK_FALSE(isValidToken("xK"));
    CHECK_FALSE(isValidToken("wX"));
    CHECK_FALSE(isValidToken("w"));
    CHECK_FALSE(isValidToken("wKQ"));
    CHECK_FALSE(isValidToken(""));
}

TEST_CASE("validateBoard throws ROW_WIDTH_MISMATCH for inconsistent row widths") {
    RawBoard raw = parseRawGrid({"wK wQ", "bK"});
    try {
        validateBoard(raw);
        FAIL("expected BoardError");
    } catch (const BoardError& e) {
        CHECK(e.code() == "ROW_WIDTH_MISMATCH");
    }
}

TEST_CASE("validateBoard throws UNKNOWN_TOKEN for invalid tokens") {
    RawBoard raw = parseRawGrid({"wK xQ"});
    try {
        validateBoard(raw);
        FAIL("expected BoardError");
    } catch (const BoardError& e) {
        CHECK(e.code() == "UNKNOWN_TOKEN");
    }
}

TEST_CASE("validateBoard accepts a well formed board") {
    RawBoard raw = parseRawGrid({"wK wQ", "bK bQ"});
    CHECK_NOTHROW(validateBoard(raw));
}

TEST_CASE("validateBoard does nothing for an empty board") {
    RawBoard raw;
    CHECK_NOTHROW(validateBoard(raw));
}

TEST_CASE("formatBoard renders rows as space separated tokens with trailing newline") {
    Board b = parseBoard({"wK . bQ", ". . ."});
    CHECK(formatBoard(b) == "wK . bQ\n. . .\n");
}

TEST_CASE("formatBoard round trips with parseBoard") {
    Board b = parseBoard({"wR wN wB", "bR bN bB"});
    CHECK(formatBoard(b) == "wR wN wB\nbR bN bB\n");
}
