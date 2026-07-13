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

bool isEmpty(const std::string& tok);
char colorOf(const std::string& tok);
char pieceOf(const std::string& tok);