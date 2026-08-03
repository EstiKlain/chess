#include "doctest.h"

#include "server/application/PlayerSessionRegistry.hpp"

// GameSession is only ever used here as an opaque pointer/key, same
// convention as ConnectionManager.tests.cpp.
class GameSession;

namespace {
// Deterministic stand-in for SodiumTokenGenerator - PlayerSessionRegistry
// tests must never depend on real randomness for assertions.
class FakeTokenGenerator : public ITokenGenerator {
public:
    std::string generate() override { return "token-" + std::to_string(++counter_); }

private:
    int counter_ = 0;
};
}  // namespace

TEST_CASE("PlayerSessionRegistry: registerNew returns unique tokens and find recovers the entry") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    const std::string tokenA = sessions.registerNew("alice", 'w', session, "conn-1");
    const std::string tokenB = sessions.registerNew("bob", 'b', session, "conn-2");
    CHECK(tokenA != tokenB);

    const auto entry = sessions.find(tokenA);
    REQUIRE(entry.has_value());
    CHECK(entry->username == "alice");
    CHECK(entry->color == 'w');
    CHECK(entry->session == session);
    REQUIRE(entry->connectionId.has_value());
    CHECK(*entry->connectionId == "conn-1");
    CHECK_FALSE(entry->disconnectedAtMs.has_value());
}

TEST_CASE("PlayerSessionRegistry: find on an unknown token returns nullopt") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);

    CHECK_FALSE(sessions.find("nonexistent").has_value());
}

TEST_CASE("PlayerSessionRegistry: markDisconnected clears connectionId and records the timestamp") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);
    const std::string token = sessions.registerNew("alice", 'w', session, "conn-1");

    sessions.markDisconnected("conn-1", 5000);

    const auto entry = sessions.find(token);
    REQUIRE(entry.has_value());
    CHECK_FALSE(entry->connectionId.has_value());
    REQUIRE(entry->disconnectedAtMs.has_value());
    CHECK(*entry->disconnectedAtMs == 5000);
}

TEST_CASE("PlayerSessionRegistry: markDisconnected on an unbound connectionId is a no-op") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);
    sessions.registerNew("alice", 'w', session, "conn-1");

    sessions.markDisconnected("conn-unknown", 5000);

    const auto entry = sessions.find("token-1");
    REQUIRE(entry.has_value());
    REQUIRE(entry->connectionId.has_value());
    CHECK(*entry->connectionId == "conn-1");
}

TEST_CASE("PlayerSessionRegistry: reconnect re-populates connectionId and clears the timestamp") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);
    const std::string token = sessions.registerNew("alice", 'w', session, "conn-1");
    sessions.markDisconnected("conn-1", 5000);

    const bool ok = sessions.reconnect(token, "conn-2");

    CHECK(ok);
    const auto entry = sessions.find(token);
    REQUIRE(entry.has_value());
    REQUIRE(entry->connectionId.has_value());
    CHECK(*entry->connectionId == "conn-2");
    CHECK_FALSE(entry->disconnectedAtMs.has_value());
}

TEST_CASE("PlayerSessionRegistry: reconnect with an unknown token fails") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);

    CHECK_FALSE(sessions.reconnect("nonexistent", "conn-2"));
}

TEST_CASE("PlayerSessionRegistry: reconnect called twice with the same token fails on the second call") {
    // Guards against a double-reconnect race (e.g. two tabs racing on one
    // saved token) - once the first reconnect succeeds, the entry is
    // connected again, so a second attempt must be rejected exactly like an
    // unknown token would be.
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);
    const std::string token = sessions.registerNew("alice", 'w', session, "conn-1");
    sessions.markDisconnected("conn-1", 5000);

    CHECK(sessions.reconnect(token, "conn-2"));
    CHECK_FALSE(sessions.reconnect(token, "conn-3"));

    const auto entry = sessions.find(token);
    REQUIRE(entry.has_value());
    REQUIRE(entry->connectionId.has_value());
    CHECK(*entry->connectionId == "conn-2");
}

TEST_CASE("PlayerSessionRegistry: disconnectedEntries lists only currently-disconnected tokens") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);
    const std::string tokenA = sessions.registerNew("alice", 'w', session, "conn-1");
    sessions.registerNew("bob", 'b', session, "conn-2");
    sessions.markDisconnected("conn-1", 5000);

    const auto disconnected = sessions.disconnectedEntries();

    REQUIRE(disconnected.size() == 1);
    CHECK(disconnected[0].first == tokenA);
    CHECK(disconnected[0].second.username == "alice");
}

TEST_CASE("PlayerSessionRegistry: usernamesFor returns both players of a session, connected or not") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);
    GameSession* otherSession = reinterpret_cast<GameSession*>(0x2);
    sessions.registerNew("alice", 'w', session, "conn-1");
    sessions.registerNew("bob", 'b', session, "conn-2");
    sessions.registerNew("carol", 'w', otherSession, "conn-3");
    sessions.markDisconnected("conn-2", 5000);  // still returned - disconnected is not "gone"

    const auto players = sessions.usernamesFor(session);

    REQUIRE(players.size() == 2);
    CHECK(((players[0].first == "alice" && players[0].second == 'w') ||
           (players[1].first == "alice" && players[1].second == 'w')));
    CHECK(((players[0].first == "bob" && players[0].second == 'b') ||
           (players[1].first == "bob" && players[1].second == 'b')));
}

TEST_CASE("PlayerSessionRegistry: usernamesFor on a session with no entries returns empty") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    CHECK(sessions.usernamesFor(session).empty());
}
