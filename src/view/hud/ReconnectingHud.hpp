#pragma once

#include "../canvas/ICanvas.hpp"

// Same free-function-in-Hud::-namespace pattern as PlayerNamesHud/
// DisconnectCountdownHud. This is the OWN-disconnect counterpart to
// DisconnectCountdownHud (which shows the OPPONENT's countdown) - the two
// are mutually exclusive by construction: if your own link is down, you are
// by definition not receiving DISCONNECT_COUNTDOWN messages about anyone
// else's, so they safely share the same screen position.
namespace Hud {

/// Shown while AutoReconnector is actively retrying a dropped connection.
void drawReconnecting(ICanvas& canvas, int secondsRemaining);

/// Shown once a reconnect episode's local deadline passed without success - a terminal state, no UI path back from it.
void drawConnectionLost(ICanvas& canvas);

}  // namespace Hud
