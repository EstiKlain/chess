#include "server/application/PlayRequestUseCase.hpp"

#include "server/application/GameSession.hpp"
#include "server/config.hpp"
#include "server/protocol/Envelope.hpp"

PlayRequestUseCase::PlayRequestUseCase(IMatchmakingQueue& queue, ConnectionManager& connections,
                                        PlayerSessionRegistry& sessions, IIdentityStore& identities,
                                        IUserRepository& users, ITransport& transport, IClock& clock,
                                        MatchStartHandler onMatch)
    : queue_(queue),
      connections_(connections),
      sessions_(sessions),
      identities_(identities),
      users_(users),
      transport_(transport),
      clock_(clock),
      onMatch_(std::move(onMatch)) {}

void PlayRequestUseCase::handlePlay(const std::string& connectionId, const std::string& requestId,
                                     const nlohmann::json&) {
    if (connections_.sessionFor(connectionId).has_value()) {
        protocol::sendError(transport_, connectionId, requestId, "ALREADY_IN_GAME", "already in a game");
        return;
    }
    if (queue_.contains(connectionId)) {
        protocol::sendError(transport_, connectionId, requestId, "ALREADY_SEARCHING", "already searching for a match");
        return;
    }

    const auto username = identities_.usernameFor(connectionId);
    if (!username.has_value()) {
        // AuthGuard only lets PLAY through for a logged-in connection, so
        // this means the connection is logged in but has no identity entry -
        // a state that should never happen, not bad input from the peer.
        protocol::sendError(transport_, connectionId, requestId, "INTERNAL_ERROR",
                             "connection is logged in but has no identity");
        return;
    }

    for (const PlayerSessionEntry& entry : sessions_.entriesForUsername(*username)) {
        if (entry.session != nullptr && !entry.session->snapshot().gameOver) {
            protocol::sendError(transport_, connectionId, requestId, "ALREADY_IN_GAME",
                                 "this account already has an active game");
            return;
        }
    }

    int elo = server_config::kInitialElo;
    if (const auto record = users_.findByUsername(*username)) {
        elo = record->elo;
    }

    const WaitingPlayer joiningPlayer{connectionId, *username, requestId, elo, clock_.nowMs()};
    if (const auto opponent = queue_.takeBestMatch(elo, server_config::kEloMatchRange)) {
        onMatch_(*opponent, joiningPlayer);
        return;
    }

    queue_.enqueue(joiningPlayer);
}

void PlayRequestUseCase::tick(long nowMs) {
    for (const WaitingPlayer& expired : queue_.takeExpired(nowMs - server_config::kMatchmakingTimeoutMs)) {
        protocol::sendError(transport_, expired.connectionId, expired.requestId, "NO_MATCH_FOUND",
                             "no opponent found within the matchmaking window");
    }
}

void PlayRequestUseCase::onDisconnected(const std::string& connectionId) { queue_.remove(connectionId); }
