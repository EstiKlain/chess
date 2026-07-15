// UI-Iteration B: draw the checkerboard AND the opening position, using
// real piece sprites. Still no GameEngine/Controller/Board here on purpose
// -- the opening layout comes from a flat CSV file that only the UI knows
// about (view/render/PiecePlacement.hpp). From Iteration D onward this
// file switches to GameEngine::snapshot() instead of the CSV.
//
// All board-drawing logic now lives in BoardRenderer (view/render), not
// here -- main_gui.cpp used to duplicate BoardRenderer::drawBoard's exact
// square-filling loop inline; that's gone, this file only orchestrates.
#include "view/canvas/ImgCanvas.hpp"
#include "view/render/BoardGeometry.hpp"
#include "view/render/BoardRenderer.hpp"
#include "view/render/PiecePlacement.hpp"
#include "view/assets/SpriteLoader.hpp"
#include "view/input/ClickLogger.hpp"
#include "config.hpp"

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

int main()
{
    const int boardRows = 8;
    const int boardCols = 8;
    const int cellSize = config::CELL_SIZE;

    const auto size = BoardGeometry::boardPixelSize(boardRows, boardCols, cellSize);
    ImgCanvas canvas(size.width, size.height, "Kung Fu Chess");

    SpriteLoader spriteLoader(std::string(PROJECT_ROOT) + "/assets/pieces2");
    const auto placements = loadOpeningFromCsv(std::string(PROJECT_ROOT) + "/assets/pieces1/board.csv");

    ClickLogger clickLogger(boardRows, boardCols);
    canvas.setOnMouseClick([&clickLogger](int x, int y)
                           { clickLogger.onClick(x, y); });

    const ColorRGB dark{181, 136, 99};

    while (!canvas.shouldClose())
    {
        canvas.clear(dark);
        BoardRenderer::drawBoard(canvas, boardRows, boardCols, cellSize);
        BoardRenderer::drawPieces(canvas, spriteLoader, placements, cellSize);
        canvas.present();
    }

    return 0;
}