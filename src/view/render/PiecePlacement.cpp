#include "PiecePlacement.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<PiecePlacement> loadOpeningFromCsv(const std::string &csvPath)
{
    std::ifstream file(csvPath);
    if (!file.is_open())
        throw std::runtime_error("Cannot open board.csv: " + csvPath);

    std::vector<PiecePlacement> placements;
    std::string line;
    int row = 0;

    while (std::getline(file, line))
    {
        // Windows line endings leave a trailing '\r' — strip it so an
        // empty final cell doesn't get treated as "RB\r" or similar.
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        std::stringstream ss(line);
        std::string cell;
        int col = 0;

        while (std::getline(ss, cell, ','))
        {
            if (!cell.empty())
                placements.push_back(PiecePlacement{cell, row, col});
            ++col;
        }

        ++row;
    }

    return placements;
}