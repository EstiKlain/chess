#pragma once

namespace EloService {

struct EloUpdate {
    int winnerNewElo;
    int loserNewElo;
};

/// Computes the standard Elo rating update for a decisive game (no draw case - GameOverReason is always KingCaptured or Resignation).
/// @param winnerElo The winner's rating before this game.
/// @param loserElo The loser's rating before this game.
/// @return Both players' new ratings.
EloUpdate apply(int winnerElo, int loserElo);

}  // namespace EloService
