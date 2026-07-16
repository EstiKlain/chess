#include "BoardRenderer.hpp"

#include <string>

#include "BoardGeometry.hpp"
#include "../assets/SpriteLoader.hpp"

namespace BoardRenderer
{
    void drawBoard(ICanvas &canvas, int boardRows, int boardCols, int cellSize)
    {
        const ColorRGB light{240, 217, 181};

        for (int row = 0; row < boardRows; ++row)
        {
            for (int col = 0; col < boardCols; ++col)
            {
                if (BoardGeometry::isLightSquare(row, col))
                {
                    const auto r = BoardGeometry::cellRect(row, col, cellSize);
                    canvas.fillRect(Rect{r.x, r.y, r.w, r.h}, light);
                }
            }
        }
    }

    void drawPieces(ICanvas &canvas,
                    SpriteLoader &spriteLoader,
                    const std::vector<PiecePlacement> &placements,
                    int cellSize)
    {
        for (const auto &placement : placements)
        {
            const auto &sprite = spriteLoader.idleSprite(placement.pieceCode, cellSize);
            const auto r = BoardGeometry::cellRect(placement.row, placement.col, cellSize);
            canvas.drawImage(sprite, r.x, r.y);
        }
    }
    void drawAnimatedPieces(ICanvas &canvas,
                            SpriteLoader &spriteLoader,
                            const std::vector<AnimatedPlacement> &placements,
                            int cellSize)
    {
        for (const auto &placement : placements)
        {
            const auto &sprite = spriteLoader.frame(placement.pieceCode, placement.state, placement.frameIndex, cellSize);
            canvas.drawImage(sprite, placement.pixelX, placement.pixelY);
        }
    }
    
    void highlightCell(ICanvas &canvas, int row, int col, int cellSize)
    {
        const ColorRGB gold{255, 215, 0};
        const int thickness = 4;
        const auto r = BoardGeometry::cellRect(row, col, cellSize);

        canvas.fillRect(Rect{r.x, r.y, r.w, thickness}, gold);                   // top
        canvas.fillRect(Rect{r.x, r.y + r.h - thickness, r.w, thickness}, gold); // bottom
        canvas.fillRect(Rect{r.x, r.y, thickness, r.h}, gold);                   // left
        canvas.fillRect(Rect{r.x + r.w - thickness, r.y, thickness, r.h}, gold); // right
    }

    void drawGameOverOverlay(ICanvas &canvas, int boardWidthPx, int boardHeightPx)
    {
        const ColorRGB white{255, 255, 255};
        const std::string text = "GAME OVER";
        const int x = boardWidthPx / 2 - 100;
        const int y = boardHeightPx / 2;
        canvas.drawText(text, x, y, white);
    }
}