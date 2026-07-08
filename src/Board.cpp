#include "Board.hpp"

#include <cctype>
#include <sstream>

std::string trim(const std::string& v) {
    size_t a = 0, b = v.size();
    while (a < b && std::isspace((unsigned char)v[a])) ++a;
    while (b > a && std::isspace((unsigned char)v[b - 1])) --b;
    return v.substr(a, b - a);
}

std::vector<std::string> splitWords(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream ss(line);
    std::string tok;
    while (ss >> tok) out.push_back(tok);
    return out;
}

Sections parseSections(const std::string& text) {
    Sections s;
    std::istringstream stream(text);
    std::string line;
    enum { NONE, BOARD, COMMANDS } where = NONE;

    while (std::getline(stream, line)) {
        std::string t = trim(line);
        if (t == "Board:")    { where = BOARD;    continue; }
        if (t == "Commands:") { where = COMMANDS; continue; }
        if (t.empty())        { continue; }
        if (where == BOARD)         s.boardLines.push_back(t);
        else if (where == COMMANDS) s.commandLines.push_back(t);
    }
    return s;
}

Board parseBoard(const std::vector<std::string>& boardLines) {
    Board b;
    for (const auto& line : boardLines) b.grid.push_back(splitWords(line));
    return b;
}

bool isValidToken(const std::string& t) {
    if (t == ".") return true;
    if (t.size() != 2) return false;
    if (t[0] != 'w' && t[0] != 'b') return false;
    switch (t[1]) {
        case 'K': case 'Q': case 'R':
        case 'B': case 'N': case 'P': return true;
        default: return false;
    }
}

void validateBoard(const Board& b) {
    if (b.grid.empty()) return;

    size_t expected = b.grid[0].size();
    for (const auto& row : b.grid)                 // structural check
        if (row.size() != expected) throw BoardError("ROW_WIDTH_MISMATCH");

    for (const auto& row : b.grid)                 // token check
        for (const auto& tok : row)
            if (!isValidToken(tok)) throw BoardError("UNKNOWN_TOKEN");
}

std::string formatBoard(const Board& b) {
    std::ostringstream out;
    for (const auto& row : b.grid) {
        for (size_t j = 0; j < row.size(); ++j) {
            if (j) out << ' ';
            out << row[j];
        }
        out << '\n';
    }
    return out.str();
}

bool isEmpty(const std::string& tok) { return tok == "."; }
char colorOf(const std::string& tok) { return tok[0]; }
char pieceOf(const std::string& tok) { return tok[1]; }

int sign(int v) { return (v > 0) - (v < 0); }

bool isPathClear(const Board& board, int fromRow, int fromCol, int toRow, int toCol) {
    int stepRow = sign(toRow - fromRow);
    int stepCol = sign(toCol - fromCol);

    int r = fromRow + stepRow, c = fromCol + stepCol;
    while (r != toRow || c != toCol) {
        if (!isEmpty(board.grid[r][c])) return false;
        r += stepRow;
        c += stepCol;
    }
    return true;
}
