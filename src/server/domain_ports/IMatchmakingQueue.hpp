#pragma once

#include <optional>
#include <string>
#include <vector>

// A player waiting for PLAY to find them an opponent.
struct WaitingPlayer {
    std::string connectionId;
    std::string username;
    std::string requestId;  // the PLAY request this player is still waiting on
    int elo = 0;
    long enqueuedAtMs = 0;
};

// Port, not just a concrete class: a process-local queue can't match players
// connected to different server instances behind a load balancer. Real
// production implementation is a Redis Sorted Set (score = Elo,
// ZRANGEBYSCORE); InMemoryMatchmakingQueue runs today behind this same port.
class IMatchmakingQueue {
public:
    virtual ~IMatchmakingQueue() = default;

    /// Adds a player to the queue. Caller's responsibility to check contains() first.
    virtual void enqueue(const WaitingPlayer& player) = 0;

    /// True if connectionId is currently queued.
    virtual bool contains(const std::string& connectionId) const = 0;

    /// Removes connectionId from the queue, if present. No-op otherwise.
    virtual void remove(const std::string& connectionId) = 0;

    /// Removes and returns the longest-waiting player within `range` Elo of
    /// `elo`, if any - the oldest wait wins, not the closest Elo, so nobody
    /// is starved by later arrivals who happen to be a closer match.
    virtual std::optional<WaitingPlayer> takeBestMatch(int elo, int range) = 0;

    /// Removes and returns everyone who enqueued at or before cutoffMs.
    virtual std::vector<WaitingPlayer> takeExpired(long cutoffMs) = 0;
};
