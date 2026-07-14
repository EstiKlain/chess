#include "BoardParser.hpp"

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

RawBoard parseRawGrid(const std::vector<std::string>& boardLines) {
    RawBoard raw;
    for (const auto& line : boardLines) raw.push_back(splitWords(line));
    return raw;
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

void validateBoard(const RawBoard& raw) {
    if (raw.empty()) return;

    size_t expected = raw[0].size();
    for (const auto& row : raw)                    // structural check
        if (row.size() != expected) throw BoardError("ROW_WIDTH_MISMATCH");

    for (const auto& row : raw)                    // token check
        for (const auto& tok : row)
            if (!isValidToken(tok)) throw BoardError("UNKNOWN_TOKEN");
}

Board buildBoard(const RawBoard& raw) {
    Board b;
    b.height = static_cast<int>(raw.size());
    b.width = raw.empty() ? 0 : static_cast<int>(raw[0].size());

    for (int r = 0; r < b.height; ++r)
        for (int c = 0; c < b.width; ++c) {
            const std::string& tok = raw[r][c];
            if (tok == ".") continue;
            b.addPiece(tok[0], tok[1], Position{r, c});
        }
    return b;
}
