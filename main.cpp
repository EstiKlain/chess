#include <cctype>
#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
// Data model — stage 1
// ============================================================

struct Board {
    std::vector<std::vector<std::string>> grid;
};

// Carries a VPL-style error code (e.g. "UNKNOWN_TOKEN").
// Thrown by validate_board, caught once in main().
class BoardError : public std::runtime_error {
public:
    explicit BoardError(const std::string& code)
        : std::runtime_error(code), code_(code) {}
    const std::string& code() const { return code_; }

private:
    std::string code_;
};

// ============================================================
// Data model — stage 2 additions
// ============================================================

struct Selection {
    bool active = false;
    int row = 0;
    int col = 0;
};

struct PieceMove {
    int fromRow, fromCol;
    int toRow, toCol;
    long startMs;
    long durationMs;
};

struct GameState {
    Board board;
    long elapsedMs = 0;
    Selection selection;
    std::vector<PieceMove> activeMoves;
};

// ============================================================
// Small string helpers
// ============================================================

std::string trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() &&
           std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::vector<std::string> splitWords(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// ============================================================
// Layer 1 — parse_sections
// ============================================================

struct Sections {
    std::vector<std::string> boardLines;
    std::vector<std::string> commandLines;
};

Sections parse_sections(const std::string& text) {
    Sections sections;
    std::istringstream stream(text);
    std::string line;
    bool inBoard = false;
    bool inCommands = false;

    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);

        if (trimmed == "Board:") {
            inBoard = true;
            inCommands = false;
            continue;
        }
        if (trimmed == "Commands:") {
            inBoard = false;
            inCommands = true;
            continue;
        }
        if (trimmed.empty()) {
            continue;
        }

        if (inBoard) {
            sections.boardLines.push_back(trimmed);
        } else if (inCommands) {
            sections.commandLines.push_back(trimmed);
        }
    }
    return sections;
}

// ============================================================
// Layer 2 — parse_board
// ============================================================

Board parse_board(const std::vector<std::string>& boardLines) {
    Board board;
    for (const std::string& line : boardLines) {
        board.grid.push_back(splitWords(line));
    }
    return board;
}

// ============================================================
// Layer 3 — validate_board
// ============================================================

bool isValidToken(const std::string& token) {
    if (token == ".") {
        return true;
    }
    if (token.size() != 2) {
        return false;
    }
    char color = token[0];
    char piece = token[1];
    if (color != 'w' && color != 'b') {
        return false;
    }
    switch (piece) {
        case 'K':
        case 'Q':
        case 'R':
        case 'B':
        case 'N':
        case 'P':
            return true;
        default:
            return false;
    }
}

void validate_board(const Board& board) {
    if (board.grid.empty()) {
        return;
    }

    size_t expectedCols = board.grid[0].size();
    for (const auto& row : board.grid) {
        if (row.size() != expectedCols) {
            throw BoardError("ROW_WIDTH_MISMATCH");
        }
    }

    for (const auto& row : board.grid) {
        for (const std::string& token : row) {
            if (!isValidToken(token)) {
                throw BoardError("UNKNOWN_TOKEN");
            }
        }
    }
}

// ============================================================
// Layer 4 — format_board
// ============================================================

std::string format_board(const Board& board) {
    std::ostringstream out;
    for (const auto& row : board.grid) {
        for (size_t j = 0; j < row.size(); ++j) {
            if (j > 0) {
                out << ' ';
            }
            out << row[j];
        }
        out << '\n';
    }
    return out.str();
}

// ============================================================
// Stage 2 — piece speed / distance helpers
//
// !!! PLACEHOLDER VALUES — NOT FROM THE ASSIGNMENT !!!
// Neither the general RTS-chess writeup nor the stage-2 spec
// gives concrete "cells per second" numbers. The writeup only
// gave a RELATIVE ordering (queen fastest, bishop/rook medium,
// knight "unique", pawn slowest, king medium). These exact
// numbers are invented so the code runs — CONFIRM against the
// real assignment/instructor before relying on them, and update
// this table once real values are known.
// ============================================================

double pieceSpeed(char pieceLetter) {
    switch (pieceLetter) {
        case 'Q': return 4.0;   // TODO: confirm real value
        case 'R': return 3.0;   // TODO: confirm real value
        case 'B': return 3.0;   // TODO: confirm real value
        case 'N': return 3.5;   // TODO: confirm real value
        case 'P': return 2.0;   // TODO: confirm real value
        case 'K': return 3.0;   // TODO: confirm real value
        default:  return 0.0;   // unreachable on a board that passed validate_board
    }
}

bool isEmpty(const std::string& token) { return token == "."; }
char colorOf(const std::string& token) { return token[0]; }

double cellDistance(int r1, int c1, int r2, int c2) {
    double dr = r2 - r1;
    double dc = c2 - c1;
    return std::sqrt(dr * dr + dc * dc);
}

// ============================================================
// Stage 2 — move resolution (called only from handleWait)
// ============================================================

void resolveMoves(GameState& state) {
    std::vector<PieceMove> stillActive;
    for (const auto& move : state.activeMoves) {
        long arrival = move.startMs + move.durationMs;
        if (state.elapsedMs >= arrival) {
            std::string piece = state.board.grid[move.fromRow][move.fromCol];
            state.board.grid[move.fromRow][move.fromCol] = ".";
            state.board.grid[move.toRow][move.toCol] = piece;
        } else {
            stillActive.push_back(move);
        }
    }
    state.activeMoves = stillActive;
}

void cancelActiveMoveFrom(GameState& state, int row, int col) {
    std::vector<PieceMove> kept;
    for (const auto& move : state.activeMoves) {
        if (!(move.fromRow == row && move.fromCol == col)) {
            kept.push_back(move);
        }
    }
    state.activeMoves = kept;
}

// ============================================================
// Stage 2 — command handlers
// ============================================================

void handleClick(GameState& state, int x, int y) {
    int col = x / 100;
    int row = y / 100;

    int rows = (int)state.board.grid.size();
    int cols = rows > 0 ? (int)state.board.grid[0].size() : 0;
    if (row < 0 || row >= rows || col < 0 || col >= cols) {
        return; // outside the board — ignored
    }

    const std::string& token = state.board.grid[row][col];

    if (!state.selection.active) {
        if (!isEmpty(token)) {
            state.selection = {true, row, col};
        }
        return; // empty cell with no selection — ignored
    }

    const std::string& selectedToken =
        state.board.grid[state.selection.row][state.selection.col];

    if (!isEmpty(token) && colorOf(token) == colorOf(selectedToken)) {
        state.selection = {true, row, col}; // friendly piece — replace selection
        return;
    }

    PieceMove move;
    move.fromRow = state.selection.row;
    move.fromCol = state.selection.col;
    move.toRow = row;
    move.toCol = col;
    move.startMs = state.elapsedMs;

    double distance = cellDistance(move.fromRow, move.fromCol, row, col);
    double speed = pieceSpeed(selectedToken[1]);
    move.durationMs = (long)(distance / speed * 1000.0);

    cancelActiveMoveFrom(state, move.fromRow, move.fromCol);
    state.activeMoves.push_back(move);

    // Design decision: selection is cleared once a move is sent.
    state.selection = Selection{};
}

void handleWait(GameState& state, long ms) {
    state.elapsedMs += ms;
    resolveMoves(state);
}

// ============================================================
// Layer 5 — run_commands (dispatch)
// ============================================================

void run_commands(const std::vector<std::string>& commands, GameState& state) {
    for (const std::string& command : commands) {
        std::istringstream stream(trim(command));
        std::string verb;
        stream >> verb;

        if (verb == "click") {
            int x, y;
            stream >> x >> y;
            handleClick(state, x, y);
        } else if (verb == "wait") {
            long ms;
            stream >> ms;
            handleWait(state, ms);
        } else if (verb == "print") {
            std::string rest;
            std::getline(stream, rest);
            if (trim(rest) == "board") {
                std::cout << format_board(state.board);
            }
        }
        // Unknown commands are silently ignored in this iteration.
    }
}

// ============================================================
// main
// ============================================================

int main() {
    std::string input;
    std::string line;
    while (std::getline(std::cin, line)) {
        input += line;
        input += '\n';
    }

    Sections sections = parse_sections(input);

    GameState state;
    state.board = parse_board(sections.boardLines);

    try {
        validate_board(state.board);
    } catch (const BoardError& error) {
        std::cout << "ERROR " << error.code() << '\n';
        return 0;
    }

    run_commands(sections.commandLines, state);
    return 0;
}