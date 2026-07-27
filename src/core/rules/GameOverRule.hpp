#pragma once
#include <optional>
#include <vector>

#include "model/Piece.hpp"

enum class GameOverReason { KingCaptured, Resignation };

/// Single source of truth for "is the game over and who won." Scans the
/// pieces captured this tick (by value snapshot, since the originals may
/// already be removed from Board by the time this is checked); if a king
/// was captured, returns the color of the OPPOSITE (surviving) king as
/// winner. Returns nullopt if the game is not over.
std::optional<char> winnerFromCaptured(const std::vector<Piece> &capturedPieces);
