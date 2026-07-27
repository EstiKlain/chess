#pragma once

#include "../canvas/ICanvas.hpp"

// Same free-function-in-Hud::-namespace pattern as PlayerNamesHud - takes
// only the plain int the view/ layer actually needs, not any server DTO.
namespace Hud {

/// Draws a single line showing how many seconds remain before the
/// disconnected opponent auto-resigns. Callers only invoke this when a
/// countdown is actually in effect (e.g. ServerConnection::
/// latestDisconnectCountdown() has a value) - this function does not
/// interpret secondsRemaining <= 0 specially, that decision belongs to the
/// caller.
void drawDisconnectCountdown(ICanvas& canvas, int secondsRemaining);

}  // namespace Hud
