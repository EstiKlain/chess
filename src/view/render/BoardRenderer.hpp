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

    // Iteration D: draws a border frame around one cell (e.g. the
    // currently-selected piece). Called AFTER drawBoard/drawPieces, on
    // top of what's already drawn - drawBoard/drawPieces themselves are
    // unchanged. Only ever draws a border (four thin fillRect strips),
    // never covers the square, so the piece underneath stays visible.
    void highlightCell(ICanvas &canvas, int row, int col, int cellSize);

    // Iteration D: draws the game-over banner. Takes plain pixel
    // dimensions (not rows/cols) since it doesn't need to know board
    // geometry, only where the center of the screen is.
    void drawGameOverOverlay(ICanvas &canvas, int boardWidthPx, int boardHeightPx);
}