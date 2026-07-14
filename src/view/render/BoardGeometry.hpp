#pragma once

// Pure math, zero dependency on Img/OpenCV/ICanvas. This is the one part
// of the view layer that is fully unit-testable without a window -- it's
// what Iteration A's tests exercise, and BoardRenderer/BoardMapper build
// on it from Iteration B onward.
//
// boardRows/boardCols are always parameters here, never a literal 8 --
// Board::rows()/cols() (model/Board.hpp) are already set at construction
// time from whatever grid was parsed, so the board can be any size. The
// same must hold once GameSnapshot exists (Iteration D): the UI reads
// rows/cols from the snapshot, it never assumes 8x8.
namespace BoardGeometry
{
    struct Size
    {
        int width;
        int height;
    };

    struct PixelRect
    {
        int x;
        int y;
        int w;
        int h;
    };

    Size boardPixelSize(int boardRows, int boardCols, int cellSize);

    PixelRect cellRect(int row, int col, int cellSize);

    bool isLightSquare(int row, int col);
}
