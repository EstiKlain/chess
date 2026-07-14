#pragma once

#include <string>
#include <vector>

#include "model/Board.hpp"

// Raw text tokens, one per cell, before any validation and before
// being turned into real Pieces. Kept as its own type because once
// tokens become Pieces there is no way left to detect malformed input
// (e.g. an uneven row width) - validation MUST happen on this raw form.
using RawBoard = std::vector<std::vector<std::string>>;

struct Sections {
    std::vector<std::string> boardLines;
    std::vector<std::string> commandLines;
};

std::string trim(const std::string& v);

std::vector<std::string> splitWords(const std::string& line);

Sections parseSections(const std::string& text);

// Splits each board line into tokens. Does NOT validate anything yet.
RawBoard parseRawGrid(const std::vector<std::string>& boardLines);

bool isValidToken(const std::string& t);

// Throws BoardError("ROW_WIDTH_MISMATCH") or BoardError("UNKNOWN_TOKEN")
// on malformed input. Must be called before buildBoard.
void validateBoard(const RawBoard& raw);

// Converts an already-validated raw grid into a real Board of Pieces.
Board buildBoard(const RawBoard& raw);
