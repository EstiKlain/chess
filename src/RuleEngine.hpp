#pragma once
#include "Board.hpp"
#include "Moves.hpp"
#include "MoveLegality.hpp"
#include "PieceRules.hpp"

MoveLegality isMoveLegal(const Board &board, const PieceMove &move, char piece, const pieceRules::PieceRulesRegistry &registry);
