#include "server/application/ServerTick.hpp"

#include "server/application/StateFanOut.hpp"

ServerTick::ServerTick(GameSessionStore& store, DisconnectUseCase& disconnectUseCase,
                        EloUpdateUseCase& eloUpdateUseCase, ConnectionManager& connections,
                        IIdentityStore& identities, ITransport& transport, IClock& clock)
    : store_(store),
      disconnectUseCase_(disconnectUseCase),
      eloUpdateUseCase_(eloUpdateUseCase),
      connections_(connections),
      identities_(identities),
      transport_(transport),
      clock_(clock) {}

void ServerTick::tick(long deltaMs) {
    advanceGameClocks(deltaMs);
    disconnectUseCase_.tick(clock_.nowMs());
    applyEloAndBroadcast();
}

void ServerTick::advanceGameClocks(long deltaMs) {
    for (GameSession* session : store_.allSessions()) {
        session->wait(deltaMs);
    }
}

void ServerTick::applyEloAndBroadcast() {
    for (GameSession* session : store_.allSessions()) {
        eloUpdateUseCase_.checkAndApply(*session);
        StateFanOut::broadcast(*session, connections_, identities_, transport_, "", "");
    }
}
