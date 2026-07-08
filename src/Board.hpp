#pragma once

#include <stdexcept>
#include <string>
#include <vector>

struct Board {
    std::vector<std::vector<std::string>> grid;
    int rows() const { return (int)grid.size(); }
    int cols() const { return grid.empty() ? 0 : (int)grid[0].size(); }
};

class BoardError : public std::runtime_error {
public:
    explicit BoardError(const std::string& code)
        : std::runtime_error(code), code_(code) {}
    const std::string& code() const { return code_; }
private:
    std::string code_;
};

struct Sections {
    std::vector<std::string> boardLines;
    std::vector<std::string> commandLines;
};

std::string trim(const std::string& v);

std::vector<std::string> splitWords(const std::string& line);

Sections parseSections(const std::string& text);

Board parseBoard(const std::vector<std::string>& boardLines);

bool isValidToken(const std::string& t);

void validateBoard(const Board& b);

std::string formatBoard(const Board& b);

bool isEmpty(const std::string& tok);
char colorOf(const std::string& tok);
char pieceOf(const std::string& tok);

int sign(int v);

bool isPathClear(const Board& board, int fromRow, int fromCol, int toRow, int toCol);
