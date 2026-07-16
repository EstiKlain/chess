// BoardRenderer only talks to ICanvas, so it can be tested with a tiny fake
// canvas that just records what it was asked to draw -- no real window.
#include "doctest.h"

#include "view/render/BoardRenderer.hpp"
#include "view/assets/SpriteLoader.hpp"

#include <string>
#include <utility>
#include <vector>
#include <functional>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

namespace
{
    class FakeCanvas : public ICanvas
    {
    public:
        int fillRectCalls = 0;
        int drawImageCalls = 0;
        int drawTextCalls = 0;
        std::vector<std::pair<int, int>> imagePositions;
        std::vector<Rect> fillRects;
        std::string lastText;

        void clear(const ColorRGB &) override {}
        void fillRect(const Rect &rect, const ColorRGB &) override
        {
            ++fillRectCalls;
            fillRects.push_back(rect);
        }
        void drawImage(const Img &, int x, int y) override
        {
            ++drawImageCalls;
            imagePositions.emplace_back(x, y);
        }
        void present() override {}
        bool shouldClose() const override { return false; }
        void setOnMouseClick(std::function<void(int, int)>) override {}
        void drawText(const std::string &text, int, int, const ColorRGB &) override
        {
            ++drawTextCalls;
            lastText = text;
        }
        int width() const override { return 800; }
        int height() const override { return 800; }
    };
}

TEST_CASE("drawBoard fills exactly the light squares of an 8x8 board")
{
    FakeCanvas canvas;
    BoardRenderer::drawBoard(canvas, 8, 8, 100);
    CHECK(canvas.fillRectCalls == 32);
}

TEST_CASE("drawBoard draws nothing for a 0x0 board")
{
    FakeCanvas canvas;
    BoardRenderer::drawBoard(canvas, 0, 0, 100);
    CHECK(canvas.fillRectCalls == 0);
}

TEST_CASE("drawPieces draws exactly one image per placement, at the right pixel position")
{
    FakeCanvas canvas;
    SpriteLoader loader(std::string(PROJECT_ROOT) + "/assets/pieces2");

    const std::vector<PiecePlacement> placements{
        {"QW", 0, 3},
        {"PB", 6, 4},
    };

    BoardRenderer::drawPieces(canvas, loader, placements, 100);

    REQUIRE(canvas.drawImageCalls == 2);
    CHECK(canvas.imagePositions[0] == std::make_pair(300, 0));
    CHECK(canvas.imagePositions[1] == std::make_pair(400, 600));
}

TEST_CASE("drawPieces draws nothing for an empty placement list")
{
    FakeCanvas canvas;
    SpriteLoader loader(std::string(PROJECT_ROOT) + "/assets/pieces2");
    BoardRenderer::drawPieces(canvas, loader, {}, 100);
    CHECK(canvas.drawImageCalls == 0);
}

// --- Iteration D --------------------------------------------------------

TEST_CASE("highlightCell draws exactly four border strips, none of them full-size")
{
    FakeCanvas canvas;
    BoardRenderer::highlightCell(canvas, 2, 3, 100);

    REQUIRE(canvas.fillRectCalls == 4);
    for (const Rect &r : canvas.fillRects)
    {
        // A border strip must never be a full 100x100 fill - that would
        // hide the piece underneath instead of framing it.
        CHECK((r.w < 100 || r.h < 100));
    }
}

TEST_CASE("highlightCell places its strips within the target cell's bounds")
{
    FakeCanvas canvas;
    BoardRenderer::highlightCell(canvas, 1, 1, 100);

    for (const Rect &r : canvas.fillRects)
    {
        CHECK(r.x >= 100);
        CHECK(r.x <= 200);
        CHECK(r.y >= 100);
        CHECK(r.y <= 200);
    }
}

TEST_CASE("drawGameOverOverlay draws text exactly once, and never touches fillRect/drawImage")
{
    FakeCanvas canvas;
    BoardRenderer::drawGameOverOverlay(canvas, 800, 800);

    CHECK(canvas.drawTextCalls == 1);
    CHECK(canvas.fillRectCalls == 0);
    CHECK(canvas.drawImageCalls == 0);
    CHECK(canvas.lastText == "GAME OVER");
}