#include "BoardRenderer.hpp"

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
}