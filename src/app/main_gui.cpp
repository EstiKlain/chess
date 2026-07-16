// UI-Iteration D: minimal playable UI, wired to the real GameEngine.
//
// This replaces the CSV opening-position + ClickLogger wiring used by
// Iterations A-C:
//   - Board comes from BoardParser (io/) reading assets/opening_board.txt,
//     exactly the same parser main_console.cpp already uses for the
//     text-mode game - not a second, UI-only board format.
//   - GameEngine::snapshot() replaces the flat CSV as the source of what
//     to draw. main_gui.cpp never reads board_/pieces_ directly.
//   - Controller::handleClick is now the ONLY input path. ClickLogger is
//     no longer constructed here (its class still exists, still compiled,
//     still covered by its own tests - it's just not wired into the real
//     game anymore; see decisions_log.md open question #3).
//
// main_gui.cpp remains a composition root only: it is the one place
// allowed to know about every layer (Board, GameEngine, Controller,
// ICanvas, BoardRenderer, SpriteLoader) and wire them together. None of
// those classes know about each other beyond the abstractions already
// established (ICanvas, GameSnapshot, std::function callbacks).
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "config.hpp"
#include "engine/GameEngine.hpp"
#include "engine/GameSnapshot.hpp"
#include "input/Controller.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "view/assets/SpriteLoader.hpp"
#include "view/canvas/ImgCanvas.hpp"
#include "view/render/BoardGeometry.hpp"
#include "view/render/BoardRenderer.hpp"
#include "view/render/SnapshotAdapter.hpp"

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

namespace
{
    std::string readFile(const std::string &path)
    {
        std::ifstream file(path);
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}

int main()
{
    const int cellSize = config::CELL_SIZE;

    // --- Board: same parser as the text-mode game, not a UI-only format ---
    const std::string boardPath = std::string(PROJECT_ROOT) + "/assets/opening_board.txt";
    const Sections sections = parseSections(readFile(boardPath));
    const RawBoard raw = parseRawGrid(sections.boardLines);

    Board board;
    try
    {
        validateBoard(raw);
        board = buildBoard(raw);
    }
    catch (const BoardError &e)
    {
        std::cerr << "Failed to load " << boardPath << ": " << e.code() << '\n';
        return 1;
    }

    // --- Engine + Controller: the real gameplay path, no CSV/ClickLogger ---
    GameEngine engine(board, pieceRules::PieceRulesRegistry());
    Controller controller(engine.board(), engine);

    // --- View setup ---
    const auto size = BoardGeometry::boardPixelSize(engine.board().rows(), engine.board().cols(), cellSize);
    ImgCanvas canvas(size.width, size.height, "Kung Fu Chess");

    SpriteLoader spriteLoader(std::string(PROJECT_ROOT) + "/assets/pieces2");

    canvas.setOnMouseClick([&controller](int x, int y)
                           { controller.handleClick(x, y); });

    const ColorRGB dark{181, 136, 99};

    auto lastFrame = std::chrono::steady_clock::now();

    while (!canvas.shouldClose())
    {
        // Real elapsed time between frames drives the arbiter (Iteration
        // D still jumps pieces straight to their target cell on arrival -
        // interpolated glide motion is Iteration E).
        const auto now = std::chrono::steady_clock::now();
        const long deltaMs = static_cast<long>( std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrame).count());
        lastFrame = now;
        engine.wait(deltaMs);

        const GameSnapshot snapshot = engine.snapshot();
        const auto placements = SnapshotAdapter::toPlacements(snapshot);

        canvas.clear(dark);
        BoardRenderer::drawBoard(canvas, snapshot.rows, snapshot.cols, cellSize);
        BoardRenderer::drawPieces(canvas, spriteLoader, placements, cellSize);

        if (controller.hasSelection())
            BoardRenderer::highlightCell(canvas, controller.selectedRow(), controller.selectedCol(), cellSize);

        if (snapshot.gameOver)
            BoardRenderer::drawGameOverOverlay(canvas, size.width, size.height);

        canvas.present();
    }

    return 0;
}
