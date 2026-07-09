#include "BoardMapper.hpp"

#include "config.hpp"

namespace BoardMapper
{
    std::optional<Position> pixelToCell(int x, int y, int boardRows, int boardCols)
    {
        if (x < 0 || y < 0)
            return std::nullopt;

        const int col = x / config::CELL_SIZE;
        const int row = y / config::CELL_SIZE;

        if (row < 0 || row >= boardRows || col < 0 || col >= boardCols)
            return std::nullopt;

        return Position{row, col};
    }
}
