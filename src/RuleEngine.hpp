#pragma once
#include "Board.hpp"
#include "Moves.hpp"
#include "MoveLegality.hpp"

MoveLegality isMoveLegal(const Board &board, const PieceMove &move, char piece);
