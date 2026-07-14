// UI-Iteration A: prove the Img/OpenCV window + draw loop works before
// touching any game logic at all. On purpose, no GameEngine, no Board, no
// Controller here -- only ICanvas and the pure BoardGeometry math get used.
#include "../view/canvas/ImgCanvas.hpp"
#include "../view/render/BoardGeometry.hpp"
#include "../config.hpp"

int main()
{
    // Iteration A only: there's no engine yet, so there's no real board to
    // ask for rows/cols. 8x8 here is a literal on purpose, scoped to this
    // file only -- from Iteration D onward this comes from
    // GameEngine::snapshot().boardRows/boardCols, never a literal again.
    const int boardRows = 8;
    const int boardCols = 8;
    const int cellSize = config::CELL_SIZE; // never re-hard-coded as 100 here

    const auto size = BoardGeometry::boardPixelSize(boardRows, boardCols, cellSize);
    ImgCanvas canvas(size.width, size.height, "Kung Fu Chess");

    const ColorRGB light{240, 217, 181};
    const ColorRGB dark{181, 136, 99};

    while (!canvas.shouldClose())
    {
        canvas.clear(dark);
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
        canvas.present();
    }

    return 0;
}
