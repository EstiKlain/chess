#include "view/hud/DisconnectCountdownHud.hpp"

#include <string>

namespace {

// Positioned well below PlayerNamesHud's lines (kFirstLineY 20, kLineHeight
// 22 - two players max out at y=42 today) so the two HUD elements never
// overlap even as more players are added later.
constexpr int kY = 90;
constexpr int kMarginX = 8;
const ColorRGB kTextColor{255, 80, 80};

}  // namespace

namespace Hud {

void drawDisconnectCountdown(ICanvas& canvas, int secondsRemaining) {
    const std::string line = "Opponent disconnected - auto-resign in " + std::to_string(secondsRemaining) + "s";
    canvas.drawText(line, kMarginX, kY, kTextColor);
}

}  // namespace Hud
