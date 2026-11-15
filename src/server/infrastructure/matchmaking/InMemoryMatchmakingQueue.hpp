#pragma once

#include <vector>

#include "server/domain_ports/IMatchmakingQueue.hpp"

// Linear scan is fine: the queue only ever holds players between PLAY and a
// match/timeout, not every connected player. No lock needed - single thread.
class InMemoryMatchmakingQueue : public IMatchmakingQueue {
public:
    void enqueue(const WaitingPlayer& player) override;
    bool contains(const std::string& connectionId) const override;
    void remove(const std::string& connectionId) override;
    std::optional<WaitingPlayer> takeBestMatch(int elo, int range) override;
    std::vector<WaitingPlayer> takeExpired(long cutoffMs) override;

private:
    std::vector<WaitingPlayer> waiting_;
};
