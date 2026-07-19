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
#include "view/assets/AnimationConfig.hpp"
#include "view/assets/SpriteLoader.hpp"
#include "view/canvas/ImgCanvas.hpp"
#include "view/render/BoardGeometry.hpp"
#include "view/render/BoardRenderer.hpp"
#include "view/render/PieceAnimator.hpp"

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

    // --- Engine + Controller: the real gameplay path ---
    GameEngine engine(board, pieceRules::PieceRulesRegistry());
    Controller controller(engine.board(), engine);

    // --- View setup ---
    const auto size = BoardGeometry::boardPixelSize(engine.board().rows(), engine.board().cols(), cellSize);
    ImgCanvas canvas(size.width, size.height, "Kung Fu Chess");

    SpriteLoader spriteLoader(std::string(PROJECT_ROOT) + "/assets/pieces_classic");

    AnimationConfigLoader animConfigLoader(std::string(PROJECT_ROOT) + "/assets/pieces_classic");
    const AnimationLookup animLookup =
        [&](const std::string &pieceCode, const std::string &state) -> AnimationSpec
    {
        const AnimationConfig &config = animConfigLoader.configFor(pieceCode, state);
        return AnimationSpec{config.framesPerSec, spriteLoader.frameCount(pieceCode, state), config.isLoop};
    };

    canvas.setOnMouseClick([&controller](int x, int y)
                           { controller.handleClick(x, y); });

    canvas.setOnRightMouseClick([&controller](int x, int y)
                                { controller.handleJumpClick(x, y); });
    const ColorRGB dark{181, 136, 99};

    auto lastFrame = std::chrono::steady_clock::now();

    while (!canvas.shouldClose())
    {
        const auto now = std::chrono::steady_clock::now();
        const long deltaMs = static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrame).count());
        lastFrame = now;
        engine.wait(deltaMs);

        const GameSnapshot snapshot = engine.snapshot();
        const auto animated = PieceAnimator::computePlacements(snapshot, cellSize, animLookup);

        canvas.clear(dark);
        BoardRenderer::drawBoard(canvas, snapshot.rows, snapshot.cols, cellSize);
        BoardRenderer::drawAnimatedPieces(canvas, spriteLoader, animated, cellSize);

        if (controller.hasSelection())
            BoardRenderer::highlightCell(canvas, controller.selectedRow(), controller.selectedCol(), cellSize);

        if (snapshot.gameOver)
            BoardRenderer::drawGameOverOverlay(canvas, size.width, size.height);

        canvas.present();
    }

    return 0;
}