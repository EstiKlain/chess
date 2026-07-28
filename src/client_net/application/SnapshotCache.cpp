#include "client_net/application/SnapshotCache.hpp"

#include <utility>

void SnapshotCache::updateFromStateUpdate(GameSnapshot snapshot, std::vector<PlayerDto> players) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot_ = std::move(snapshot);
        players_ = std::move(players);
        hasSnapshot_ = true;
        if (snapshot_.gameOver) {
            disconnectCountdownSeconds_ = std::nullopt;
        }
    }
    snapshotCv_.notify_all();
}

GameSnapshot SnapshotCache::awaitInitialSnapshot() {
    std::unique_lock<std::mutex> lock(mutex_);
    snapshotCv_.wait(lock, [this] { return hasSnapshot_; });
    return snapshot_;
}

GameSnapshot SnapshotCache::latestSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

std::vector<PlayerDto> SnapshotCache::latestPlayers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return players_;
}

void SnapshotCache::setSessionToken(std::string token) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessionToken_ = std::move(token);
}

std::optional<std::string> SnapshotCache::sessionToken() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessionToken_;
}

void SnapshotCache::updateDisconnectCountdown(int secondsLeft) {
    std::lock_guard<std::mutex> lock(mutex_);
    disconnectCountdownSeconds_ = (secondsLeft > 0) ? std::optional<int>(secondsLeft) : std::nullopt;
}

void SnapshotCache::clearDisconnectCountdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    disconnectCountdownSeconds_ = std::nullopt;
}

std::optional<int> SnapshotCache::latestDisconnectCountdown() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return disconnectCountdownSeconds_;
}
