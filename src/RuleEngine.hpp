#pragma once
#include "Board.hpp"
#include "GameState.hpp"
#include "MoveLegality.hpp"

MoveLegality isMoveLegal(const Board &board, const PieceMove &move, char piece);
