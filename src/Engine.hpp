#pragma once

#include <string>
#include <vector>

#include "GameState.hpp"

void resolveMoves(GameState& st);

void sendMove(GameState& st, int player, int toRow, int toCol);

void handleClick(GameState& st, int player, int x, int y);

void handleWait(GameState& st, long ms);

void runCommands(const std::vector<std::string>& commands, GameState& st);
