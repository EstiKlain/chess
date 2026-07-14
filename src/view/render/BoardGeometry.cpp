#include "BoardGeometry.hpp"

namespace BoardGeometry
{
    Size boardPixelSize(int boardRows, int boardCols, int cellSize)
    {
        return Size{boardCols * cellSize, boardRows * cellSize};
    }

    PixelRect cellRect(int row, int col, int cellSize)
    {
        return PixelRect{col * cellSize, row * cellSize, cellSize, cellSize};
    }

    bool isLightSquare(int row, int col)
    {
        return (row + col) % 2 == 0;
    }
}
