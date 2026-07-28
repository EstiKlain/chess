#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "engine/GameSnapshot.hpp"
#include "shared/protocol/dto/StateUpdateDto.hpp"

// Owns the "what should currently be displayed" read-model: the latest
// snapshot/player roster, the login sessionToken, and the opponent's
// disconnect countdown. Split out of ServerConnection so that class stays
// focused on protocol interpretation (envelope dispatch, login/reconnect
// state machines) rather than also being the render loop's data cache -
// two different responsibilities that don't need to share one class just
// because they used to be written that way. Thread-safe on its own: written
// from whatever thread ServerConnection::onMessage runs on, read from the
// render-loop thread.
class SnapshotCache {
public:
    /// Stores a freshly-received snapshot/roster. Also clears the disconnect countdown if the game is now over - a countdown never outlives the game, regardless of why it ended (king capture or an auto-resign timeout).
    void updateFromStateUpdate(GameSnapshot snapshot, std::vector<PlayerDto> players);

    /// Blocks until at least one snapshot has been stored.
    GameSnapshot awaitInitialSnapshot();

    GameSnapshot latestSnapshot() const;
    std::vector<PlayerDto> latestPlayers() const;

    void setSessionToken(std::string token);
    std::optional<std::string> sessionToken() const;

    /// secondsLeft <= 0 clears the countdown - the server's explicit "the countdown is over" signal (sent by ReconnectUseCase on a successful reconnect).
    void updateDisconnectCountdown(int secondsLeft);

    /// Clears the countdown unconditionally, regardless of its current value - used when OUR OWN link drops (a stale countdown about the opponent becomes meaningless the moment we can't talk to the server ourselves either).
    void clearDisconnectCountdown();

    std::optional<int> latestDisconnectCountdown() const;

private:
    mutable std::mutex mutex_;
    std::condition_variable snapshotCv_;
    GameSnapshot snapshot_;
    std::vector<PlayerDto> players_;
    bool hasSnapshot_ = false;
    std::optional<std::string> sessionToken_;
    std::optional<int> disconnectCountdownSeconds_;
};
