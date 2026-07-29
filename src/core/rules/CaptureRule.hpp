#pragma once

/// Pure chess rule: does a piece of moverColor arriving on a cell occupied
/// by residentColor result in a capture? Mirrors GameOverRule.hpp's own
/// pattern - a small pure function in rules/, not inline color-comparison
/// logic. The same question is asked both when a move lands on an
/// occupied destination (RealTimeArbiter::resolveMoves) and when a
/// mid-flight collision needs to decide a winner (see CollisionRule.hpp).
bool capturesOnArrival(char moverColor, char residentColor);
