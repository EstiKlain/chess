#pragma once

#include <string>
#include <vector>

#include "GameState.hpp"
#include "MoveRequest.hpp"

void sendMove(GameState &st, const MoveRequest &request);

void handleClick(GameState &st, int x, int y);

void handleWait(GameState &st, long ms);

void runCommands(const std::vector<std::string> &commands, GameState &st);

void sendJump(GameState &st, int row, int col);

void handleJump(GameState &st, int x, int y);