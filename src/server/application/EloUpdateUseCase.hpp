#pragma once

#include <unordered_set>

#include "server/application/ConnectionManager.hpp"
#include "server/application/GameSession.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/domain_ports/IUserRepository.hpp"

// Detects a game ending and applies the resulting Elo update. Called from
// main_server.cpp's tick loop, after disconnectUseCase.tick() - NOT a bus
// subscriber. GameEngine::wait() (called via GameSession::wait, only ever
// from the tick thread) is the only place gameOver_ ever flips to true -
// the "MoveApplied" bus event fires the instant a move is accepted, before
// a nonzero-duration move's king-capture actually resolves, so a
// subscriber on it would frequently miss the ending. Positioned after
// disconnectUseCase.tick() so a same-tick auto-resign is visible too - one
// call site catches both a king-capture and a disconnect-timeout
// resignation, with no new bus event needed.
class EloUpdateUseCase {
public:
    EloUpdateUseCase(IUserRepository& users, PlayerSessionRegistry& sessions, ConnectionManager& connections);

    /// No-op unless session just ended, is rating-eligible, and hasn't already been processed. Idempotency is required, not optional - GameSnapshot::gameOver stays true on every tick forever after the game ends, so without the guard this would recompute and re-persist Elo every 50ms indefinitely.
    /// @param session The session to check.
    void checkAndApply(GameSession& session);

private:
    IUserRepository& users_;
    PlayerSessionRegistry& sessions_;
    ConnectionManager& connections_;
    std::unordered_set<const GameSession*> processed_;
};
