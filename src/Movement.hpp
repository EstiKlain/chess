#pragma once

#include "Board.hpp"
#include "Moves.hpp"
#include "MoveLegality.hpp"
#include "PieceRules.hpp"

int playerIndexOf(char color);

double cellDistance(int r1, int c1, int r2, int c2);

MoveLegality checkPieceShape(const Board &board, const PieceMove &move, char piece, const pieceRules::PieceRulesRegistry &registry);
