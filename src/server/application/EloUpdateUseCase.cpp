#include "server/application/EloUpdateUseCase.hpp"

#include "server/application/EloService.hpp"

EloUpdateUseCase::EloUpdateUseCase(IUserRepository& users, PlayerSessionRegistry& sessions,
                                    ConnectionManager& connections)
    : users_(users), sessions_(sessions), connections_(connections) {}

void EloUpdateUseCase::checkAndApply(GameSession& session) {
    const GameSnapshot snapshot = session.snapshot();
    if (!snapshot.gameOver || !session.ratingEligible() || processed_.count(&session) || !snapshot.winner.has_value()) {
        return;
    }
    processed_.insert(&session);

    const char winnerColor = *snapshot.winner;
    std::optional<std::string> winnerUsername;
    std::optional<std::string> loserUsername;
    for (const auto& [username, color] : sessions_.usernamesFor(&session)) {
        if (color == winnerColor) {
            winnerUsername = username;
        } else {
            loserUsername = username;
        }
    }
    if (!winnerUsername.has_value() || !loserUsername.has_value()) {
        return;
    }

    const auto winnerRecord = users_.findByUsername(*winnerUsername);
    const auto loserRecord = users_.findByUsername(*loserUsername);
    if (!winnerRecord.has_value() || !loserRecord.has_value()) {
        return;
    }

    const EloService::EloUpdate update = EloService::apply(winnerRecord->elo, loserRecord->elo);
    users_.updateElo(*winnerUsername, update.winnerNewElo);
    users_.updateElo(*loserUsername, update.loserNewElo);

    for (const auto& [connectionId, binding] : connections_.connectionsFor(&session)) {
        connections_.setElo(connectionId, binding.color == winnerColor ? update.winnerNewElo : update.loserNewElo);
    }
}
