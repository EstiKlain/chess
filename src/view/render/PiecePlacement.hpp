#pragma once

#include <string>
#include <vector>

struct PiecePlacement
{
    std::string pieceCode; // e.g. "QW", "PB" -- matches the assets folder name exactly
    int row;
    int col;
};

std::vector<PiecePlacement> loadOpeningFromCsv(const std::string &csvPath);