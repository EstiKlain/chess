#include "server/application/EloService.hpp"

#include <cmath>

#include "server/config.hpp"

namespace {

// Standard Elo formula constants (Arpad Elo's original system) - unlike
// kEloKFactor (a tunable game-balance parameter), these define the formula
// itself and are never expected to change.
constexpr double kEloScaleFactor = 400.0;  // rating-difference-to-probability scale
constexpr double kEloBase = 10.0;          // logistic curve base

// Actual-score inputs to the formula - this game has no draw outcome
// (GameOverReason is always KingCaptured or Resignation), so the winner's
// actual score is always a full win and the loser's a full loss.
constexpr double kDecisiveWinScore = 1.0;
constexpr double kDecisiveLossScore = 0.0;

double expectedScore(int self, int other) {
    return 1.0 / (1.0 + std::pow(kEloBase, (other - self) / kEloScaleFactor));
}

int updatedRating(int self, int other, double actualScore) {
    const double expected = expectedScore(self, other);
    return static_cast<int>(std::lround(self + server_config::kEloKFactor * (actualScore - expected)));
}

}  // namespace

namespace EloService {

EloUpdate apply(int winnerElo, int loserElo) {
    return EloUpdate{updatedRating(winnerElo, loserElo, kDecisiveWinScore),
                      updatedRating(loserElo, winnerElo, kDecisiveLossScore)};
}

}  // namespace EloService
