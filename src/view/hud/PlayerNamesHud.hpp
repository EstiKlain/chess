#pragma once

#include <string>
#include <vector>

#include "../canvas/ICanvas.hpp"

// view/ input type, deliberately not server::protocol's PlayerDto - view/
// only ever consumes plain data shaped for its own needs, same invariant
// as "renderer never receives a live Board/Piece, only GameSnapshot"
// (docs/kungfu_chess_ui_plan.md). Keeps view/ decoupled from any
// server/session concept; the one-line conversion from PlayerDto lives in
// main_gui.cpp, the only place that already depends on both.
struct PlayerName {
    std::string color;
    std::string name;
};

namespace Hud {

/// Draws every player's name, stacked one per line - loops over players.size(), never assumes exactly 2. players[] was deliberately designed N-player-ready in Server-Iteration 3; this function must not silently reintroduce a fixed-2 assumption.
void drawPlayerNames(ICanvas& canvas, const std::vector<PlayerName>& players);

}  // namespace Hud
