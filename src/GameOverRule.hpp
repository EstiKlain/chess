#pragma once
#include <vector>

#include "model/Piece.hpp"

// Takes the list of pieces captured this tick (by value snapshot,
// since the originals may already be removed from Board by the time
// this is checked).
bool isGameOver(const std::vector<Piece> &capturedPieces);
