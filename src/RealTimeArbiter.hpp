#pragma once

#include "Board.hpp"
#include "GameState.hpp"

std::vector<std::string> resolveMoves(GameState &st);

bool isPieceInFlight(const GameState &st, int row, int col);

bool hasActiveMotion(const GameState &st);
