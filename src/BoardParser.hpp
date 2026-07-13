#pragma once

#include <string>
#include <vector>

#include "Board.hpp"

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