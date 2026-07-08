#include "doctest.h"

#include "Board.hpp"

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

TEST_CASE("parseBoard builds a grid of tokens from board lines") {
    Board b = parseBoard({"wR wN", "bR bN"});
    REQUIRE(b.rows() == 2);
    REQUIRE(b.cols() == 2);
    CHECK(b.grid[0][0] == "wR");
    CHECK(b.grid[0][1] == "wN");
    CHECK(b.grid[1][0] == "bR");
    CHECK(b.grid[1][1] == "bN");
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
    Board b = parseBoard({"wK wQ", "bK"});
    try {
        validateBoard(b);
        FAIL("expected BoardError");
    } catch (const BoardError& e) {
        CHECK(e.code() == "ROW_WIDTH_MISMATCH");
    }
}

TEST_CASE("validateBoard throws UNKNOWN_TOKEN for invalid tokens") {
    Board b = parseBoard({"wK xQ"});
    try {
        validateBoard(b);
        FAIL("expected BoardError");
    } catch (const BoardError& e) {
        CHECK(e.code() == "UNKNOWN_TOKEN");
    }
}

TEST_CASE("validateBoard accepts a well formed board") {
    Board b = parseBoard({"wK wQ", "bK bQ"});
    CHECK_NOTHROW(validateBoard(b));
}

TEST_CASE("validateBoard does nothing for an empty board") {
    Board b;
    CHECK_NOTHROW(validateBoard(b));
}

TEST_CASE("formatBoard renders rows as space separated tokens with trailing newline") {
    Board b = parseBoard({"wK . bQ", ". . ."});
    CHECK(formatBoard(b) == "wK . bQ\n. . .\n");
}

TEST_CASE("formatBoard round trips with parseBoard") {
    Board b = parseBoard({"wR wN wB", "bR bN bB"});
    CHECK(formatBoard(b) == "wR wN wB\nbR bN bB\n");
}

TEST_CASE("isEmpty/colorOf/pieceOf read a single token") {
    CHECK(isEmpty("."));
    CHECK_FALSE(isEmpty("wK"));
    CHECK(colorOf("wK") == 'w');
    CHECK(colorOf("bQ") == 'b');
    CHECK(pieceOf("wK") == 'K');
    CHECK(pieceOf("bP") == 'P');
}

TEST_CASE("sign returns -1, 0 or 1") {
    CHECK(sign(5) == 1);
    CHECK(sign(-5) == -1);
    CHECK(sign(0) == 0);
}

TEST_CASE("isPathClear allows adjacent squares with nothing in between") {
    Board b = parseBoard({"wK .", ". bK"});
    CHECK(isPathClear(b, 0, 0, 1, 1));
}

TEST_CASE("isPathClear returns true when all intermediate squares are empty") {
    Board b = parseBoard({"wR . . bR"});
    CHECK(isPathClear(b, 0, 0, 0, 3));
}

TEST_CASE("isPathClear returns false when a piece blocks the path") {
    Board b = parseBoard({"wR . wP bR"});
    CHECK_FALSE(isPathClear(b, 0, 0, 0, 3));
}

TEST_CASE("isPathClear works along diagonals") {
    Board b = parseBoard({
        "wB . . .",
        ". . . .",
        ". . bP .",
        ". . . bB"
    });
    CHECK_FALSE(isPathClear(b, 0, 0, 3, 3));
}
