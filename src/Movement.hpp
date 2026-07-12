#pragma once

#include "Board.hpp"
#include "GameState.hpp"
#include "MoveLegality.hpp"

int playerIndexOf(char color);

double cellDistance(int r1, int c1, int r2, int c2);

MoveLegality isLegalMove(const Board& board, const PieceMove& move, char piece);
