#pragma once

#include <vector>

#include "PiecePlacement.hpp"
#include "../canvas/ICanvas.hpp"

class SpriteLoader;

namespace BoardRenderer
{
    void drawBoard(ICanvas &canvas, int boardRows, int boardCols, int cellSize);

    void drawPieces(ICanvas &canvas,
                     SpriteLoader &spriteLoader,
                     const std::vector<PiecePlacement> &placements,
                     int cellSize);
}