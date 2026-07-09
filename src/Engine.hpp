#pragma once

#include <string>
#include <vector>

#include "GameState.hpp"

void resolveMoves(GameState& st);

void sendMove(GameState& st, int toRow, int toCol);

void handleClick(GameState& st, int x, int y);

void handleWait(GameState& st, long ms);

void runCommands(const std::vector<std::string>& commands, GameState& st);

bool isPieceInFlight(const GameState& st, int row, int col);