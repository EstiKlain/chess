#include "view/hud/PlayerNamesHud.hpp"

#include <cstddef>

namespace {

constexpr int kFirstLineY = 20;
constexpr int kLineHeight = 22;
constexpr int kMarginX = 8;
const ColorRGB kTextColor{255, 255, 255};

}  // namespace

namespace Hud {

void drawPlayerNames(ICanvas& canvas, const std::vector<PlayerName>& players) {
    for (std::size_t i = 0; i < players.size(); ++i) {
        const std::string line = players[i].color + ": " + players[i].name;
        canvas.drawText(line, kMarginX, kFirstLineY + static_cast<int>(i) * kLineHeight, kTextColor);
    }
}

}  // namespace Hud
