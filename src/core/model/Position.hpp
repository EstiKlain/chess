#pragma once
#include <string>
#include <ostream>

struct Position
{
    int row = 0;
    int col = 0;

    bool operator==(const Position &other) const
    {
        return row == other.row && col == other.col;
    }

    std::string toString() const
    {
        return "(" + std::to_string(row) + "," + std::to_string(col) + ")";
    }

    friend std::ostream& operator<<(std::ostream& os, const Position& pos)
    {
        os << pos.toString();
        return os;
    }
};
