#pragma once

#include "Board.hpp"
#include "GameState.hpp"

void resolveMoves(GameState &st);

bool isPieceInFlight(const GameState &st, int row, int col);

bool hasActiveMotion(const GameState &st);
