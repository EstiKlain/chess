#pragma once

#include "server/application/ConnectionManager.hpp"
#include "server/application/GameSessionStore.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/domain_ports/IMatchmakingQueue.hpp"
#include "server/domain_ports/ITransport.hpp"

// Free function, not a class - mirrors StateFanOut: pure orchestration, no
// state between calls.
namespace MatchStarter {

/// Creates a new GameSession for two matched players, binds both connections, registers both for reconnect, and sends each their own MATCH_FOUND. waitingPlayer gets white, joiningPlayer gets black.
void start(GameSessionStore& store, ConnectionManager& connections, PlayerSessionRegistry& sessions,
           ITransport& transport, const WaitingPlayer& waitingPlayer, const WaitingPlayer& joiningPlayer);

}  // namespace MatchStarter
