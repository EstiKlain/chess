#pragma once

#include "Board.hpp"
#include "Moves.hpp"
#include "MoveLegality.hpp"

int playerIndexOf(char color);

double cellDistance(int r1, int c1, int r2, int c2);

MoveLegality checkPieceShape(const Board& board, const PieceMove& move, char piece);

// Temporary home (Stage 1): moved out of Board so Board stays model-only.
// Will move again into PieceRules in Stage 5.
int sign(int v);
 
bool isPathClear(const Board& board, int fromRow, int fromCol, int toRow, int toCol);