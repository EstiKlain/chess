#include "doctest.h"

#include "server/infrastructure/matchmaking/InMemoryMatchmakingQueue.hpp"

TEST_CASE("InMemoryMatchmakingQueue: takeBestMatch respects the range boundary exactly") {
    InMemoryMatchmakingQueue queue;
    queue.enqueue(WaitingPlayer{"conn-1", "alice", "req-1", /*elo=*/1300, /*enqueuedAtMs=*/1000});

    // Exactly at the boundary (elo+100) must still match.
    const auto atBoundary = queue.takeBestMatch(1200, 100);
    REQUIRE(atBoundary.has_value());
    CHECK(atBoundary->connectionId == "conn-1");

    queue.enqueue(WaitingPlayer{"conn-2", "bob", "req-2", /*elo=*/1301, /*enqueuedAtMs=*/1000});

    // One point past the boundary must not match.
    CHECK_FALSE(queue.takeBestMatch(1200, 100).has_value());
}

TEST_CASE("InMemoryMatchmakingQueue: takeBestMatch picks the longest-waiting player in range, not the closest Elo") {
    InMemoryMatchmakingQueue queue;
    // carol is out of range entirely.
    queue.enqueue(WaitingPlayer{"conn-carol", "carol", "req-carol", 1350, 1000});
    // alice is a worse Elo match than bob but enqueued first.
    queue.enqueue(WaitingPlayer{"conn-alice", "alice", "req-alice", 1200, 2000});
    queue.enqueue(WaitingPlayer{"conn-bob", "bob", "req-bob", 1250, 5000});

    const auto match = queue.takeBestMatch(1220, 100);

    REQUIRE(match.has_value());
    CHECK(match->connectionId == "conn-alice");

    // alice is gone; carol is still out of range; bob is now the only candidate left.
    const auto second = queue.takeBestMatch(1220, 100);
    REQUIRE(second.has_value());
    CHECK(second->connectionId == "conn-bob");
}

TEST_CASE("InMemoryMatchmakingQueue: takeBestMatch returns nullopt when nobody is in range") {
    InMemoryMatchmakingQueue queue;
    queue.enqueue(WaitingPlayer{"conn-1", "alice", "req-1", 1900, 1000});

    CHECK_FALSE(queue.takeBestMatch(1200, 100).has_value());
}

TEST_CASE("InMemoryMatchmakingQueue: contains and remove") {
    InMemoryMatchmakingQueue queue;
    queue.enqueue(WaitingPlayer{"conn-1", "alice", "req-1", 1200, 1000});

    CHECK(queue.contains("conn-1"));
    CHECK_FALSE(queue.contains("conn-unknown"));

    queue.remove("conn-1");

    CHECK_FALSE(queue.contains("conn-1"));
    CHECK_FALSE(queue.takeBestMatch(1200, 1000).has_value());
}

TEST_CASE("InMemoryMatchmakingQueue: remove on an unknown connectionId is a no-op") {
    InMemoryMatchmakingQueue queue;
    queue.enqueue(WaitingPlayer{"conn-1", "alice", "req-1", 1200, 1000});

    queue.remove("conn-unknown");  // must not throw or remove anything

    CHECK(queue.contains("conn-1"));
}

TEST_CASE("InMemoryMatchmakingQueue: takeExpired only removes and returns players who enqueued at or before cutoff") {
    InMemoryMatchmakingQueue queue;
    queue.enqueue(WaitingPlayer{"conn-old", "alice", "req-1", 1200, /*enqueuedAtMs=*/5000});
    queue.enqueue(WaitingPlayer{"conn-boundary", "bob", "req-2", 1200, /*enqueuedAtMs=*/6000});
    queue.enqueue(WaitingPlayer{"conn-fresh", "carol", "req-3", 1200, /*enqueuedAtMs=*/7000});

    const auto expired = queue.takeExpired(6000);

    REQUIRE(expired.size() == 2);
    CHECK((expired[0].connectionId == "conn-old" || expired[1].connectionId == "conn-old"));
    CHECK((expired[0].connectionId == "conn-boundary" || expired[1].connectionId == "conn-boundary"));

    // Expired players are gone; the fresh one remains queued.
    CHECK_FALSE(queue.contains("conn-old"));
    CHECK_FALSE(queue.contains("conn-boundary"));
    CHECK(queue.contains("conn-fresh"));
}

TEST_CASE("InMemoryMatchmakingQueue: takeExpired returns nothing when nobody has expired") {
    InMemoryMatchmakingQueue queue;
    queue.enqueue(WaitingPlayer{"conn-1", "alice", "req-1", 1200, 7000});

    CHECK(queue.takeExpired(6000).empty());
    CHECK(queue.contains("conn-1"));
}
