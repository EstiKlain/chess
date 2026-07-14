#pragma once
#include "model/Board.hpp"
#include "realtime/Moves.hpp"
#include "rules/MoveLegality.hpp"
#include "rules/PieceRules.hpp"

MoveLegality isMoveLegal(const Board &board, const PieceMove &move, char piece, const pieceRules::PieceRulesRegistry &registry);
