#pragma once

#include "server/application/ConnectionManager.hpp"
#include "server/application/DisconnectUseCase.hpp"
#include "server/application/EloUpdateUseCase.hpp"
#include "server/application/GameSessionStore.hpp"
#include "server/domain_ports/IClock.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

// Drives one server tick across every GameSession: advances each session's
// clock, runs the global disconnect-timeout check, then applies Elo and
// broadcasts STATE_UPDATE per session. Order matters throughout - wait()
// before disconnectUseCase_.tick() (a same-tick king-capture must win over a
// same-tick disconnect-resign), and Elo before broadcast (so the same
// STATE_UPDATE reflects a just-applied rating change).
class ServerTick {
public:
    ServerTick(GameSessionStore& store, DisconnectUseCase& disconnectUseCase, EloUpdateUseCase& eloUpdateUseCase,
               ConnectionManager& connections, IIdentityStore& identities, ITransport& transport, IClock& clock);

    void tick(long deltaMs);

private:
    void advanceGameClocks(long deltaMs);
    void applyEloAndBroadcast();

    GameSessionStore& store_;
    DisconnectUseCase& disconnectUseCase_;
    EloUpdateUseCase& eloUpdateUseCase_;
    ConnectionManager& connections_;
    IIdentityStore& identities_;
    ITransport& transport_;
    IClock& clock_;
};
