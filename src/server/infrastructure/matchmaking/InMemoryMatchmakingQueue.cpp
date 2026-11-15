#include "server/infrastructure/matchmaking/InMemoryMatchmakingQueue.hpp"

#include <algorithm>
#include <cstdlib>

void InMemoryMatchmakingQueue::enqueue(const WaitingPlayer& player) { waiting_.push_back(player); }

bool InMemoryMatchmakingQueue::contains(const std::string& connectionId) const {
    return std::any_of(waiting_.begin(), waiting_.end(),
                        [&](const WaitingPlayer& player) { return player.connectionId == connectionId; });
}

void InMemoryMatchmakingQueue::remove(const std::string& connectionId) {
    waiting_.erase(std::remove_if(waiting_.begin(), waiting_.end(),
                                   [&](const WaitingPlayer& player) { return player.connectionId == connectionId; }),
                    waiting_.end());
}

std::optional<WaitingPlayer> InMemoryMatchmakingQueue::takeBestMatch(int elo, int range) {
    auto best = waiting_.end();
    for (auto it = waiting_.begin(); it != waiting_.end(); ++it) {
        if (std::abs(it->elo - elo) > range) {
            continue;
        }
        if (best == waiting_.end() || it->enqueuedAtMs < best->enqueuedAtMs) {
            best = it;
        }
    }
    if (best == waiting_.end()) {
        return std::nullopt;
    }
    WaitingPlayer result = *best;
    waiting_.erase(best);
    return result;
}

std::vector<WaitingPlayer> InMemoryMatchmakingQueue::takeExpired(long cutoffMs) {
    std::vector<WaitingPlayer> expired;
    auto it = std::partition(waiting_.begin(), waiting_.end(),
                              [&](const WaitingPlayer& player) { return player.enqueuedAtMs > cutoffMs; });
    for (auto expiredIt = it; expiredIt != waiting_.end(); ++expiredIt) {
        expired.push_back(*expiredIt);
    }
    waiting_.erase(it, waiting_.end());
    return expired;
}
