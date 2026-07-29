#pragma once

#include <vector>

#include "server/domain_ports/IMatchmakingQueue.hpp"

// Concrete IMatchmakingQueue: a process-local std::vector<WaitingPlayer>,
// scanned linearly. Acceptable because the queue only ever holds players
// currently between PLAY and a match/timeout (tens at once, at most) - not
// every connected player - and every scan here runs on the single
// io_context thread, so no lock is needed either.
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
