#include "server/application/MatchStarter.hpp"

#include "server/protocol/Envelope.hpp"
#include "shared/protocol/dto/MatchFoundDto.hpp"

namespace {

void bindAndNotify(GameSession& session, ConnectionManager& connections, PlayerSessionRegistry& sessions,
                    ITransport& transport, char color, const WaitingPlayer& player, const WaitingPlayer& opponent) {
    connections.bindKnown(player.connectionId, &session, color);
    connections.setElo(player.connectionId, player.elo);
    const std::string token = sessions.registerNew(player.username, color, &session, player.connectionId);

    transport.send(player.connectionId,
                    protocol::envelope("MATCH_FOUND", player.requestId,
                                        nlohmann::json(MatchFoundDto{token, color, {opponent.username, opponent.elo}})));
}

}  // namespace

namespace MatchStarter {

void start(GameSessionStore& store, ConnectionManager& connections, PlayerSessionRegistry& sessions,
           ITransport& transport, const WaitingPlayer& waitingPlayer, const WaitingPlayer& joiningPlayer) {
    GameSession& session = store.create(/*ratingEligible=*/true);

    bindAndNotify(session, connections, sessions, transport, 'w', waitingPlayer, joiningPlayer);
    bindAndNotify(session, connections, sessions, transport, 'b', joiningPlayer, waitingPlayer);
}

}  // namespace MatchStarter
