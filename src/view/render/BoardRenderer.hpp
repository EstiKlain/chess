#pragma once

#include <vector>

#include "PiecePlacement.hpp"
#include "PieceAnimator.hpp"
#include "../canvas/ICanvas.hpp"

class SpriteLoader;

namespace BoardRenderer
{
    void drawBoard(ICanvas &canvas, int boardRows, int boardCols, int cellSize);

    void drawPieces(ICanvas &canvas,
                    SpriteLoader &spriteLoader,
                    const std::vector<PiecePlacement> &placements,
                    int cellSize);

    void drawAnimatedPieces(ICanvas &canvas,
                            SpriteLoader &spriteLoader,
                            const std::vector<AnimatedPlacement> &placements,
                            int cellSize);

    void highlightCell(ICanvas &canvas, int row, int col, int cellSize);

    void drawGameOverOverlay(ICanvas &canvas, int boardWidthPx, int boardHeightPx);
}