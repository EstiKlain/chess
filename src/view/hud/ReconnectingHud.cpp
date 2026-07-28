#include "view/hud/ReconnectingHud.hpp"

#include <string>

namespace {

// Same slot as DisconnectCountdownHud's kY - never on-screen at the same
// time as that HUD (see this file's own header comment for why).
constexpr int kY = 90;
constexpr int kMarginX = 8;
const ColorRGB kTextColor{255, 80, 80};

}  // namespace

namespace Hud {

void drawReconnecting(ICanvas& canvas, int secondsRemaining) {
    const std::string line = "Reconnecting... " + std::to_string(secondsRemaining) + "s";
    canvas.drawText(line, kMarginX, kY, kTextColor);
}

void drawConnectionLost(ICanvas& canvas) {
    canvas.drawText("Connection lost", kMarginX, kY, kTextColor);
}

}  // namespace Hud
