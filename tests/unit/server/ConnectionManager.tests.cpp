#include "doctest.h"

#include "server/application/ConnectionManager.hpp"
#include "server/application/PlayerSessionRegistry.hpp"

// GameSession is only ever used here as an opaque pointer/key - forward
// declared in ConnectionManager.hpp, and ConnectionManager never dereferences
// it. No real GameSession construction (and therefore no GameEngine) needed
// for these tests.
class GameSession;

namespace {
class FakeTokenGenerator : public ITokenGenerator {
public:
    std::string generate() override { return "token-" + std::to_string(++counter_); }

private:
    int counter_ = 0;
};
}  // namespace

TEST_CASE("ConnectionManager: first connection gets 'w', second gets 'b'") {
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    const ConnectionOutcome first = connections.onConnected("conn-1", session, sessions);
    const ConnectionOutcome second = connections.onConnected("conn-2", session, sessions);

    CHECK(first.accepted);
    CHECK(first.color == 'w');
    CHECK(second.accepted);
    CHECK(second.color == 'b');
    CHECK(connections.connectionCount() == 2);
}

TEST_CASE("ConnectionManager: a third connection is rejected once both colors are taken") {
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    connections.onConnected("conn-1", session, sessions);
    connections.onConnected("conn-2", session, sessions);
    const ConnectionOutcome third = connections.onConnected("conn-3", session, sessions);

    CHECK_FALSE(third.accepted);
    CHECK(connections.connectionCount() == 2);
}

TEST_CASE("ConnectionManager: disconnecting frees a color for the next connection") {
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    connections.onConnected("conn-1", session, sessions);
    connections.onConnected("conn-2", session, sessions);
    connections.onDisconnected("conn-1");
    const ConnectionOutcome rejoin = connections.onConnected("conn-3", session, sessions);

    CHECK(rejoin.accepted);
    CHECK(rejoin.color == 'w');
}

TEST_CASE("ConnectionManager: sessionFor and connectionsFor reflect the bindings made") {
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    connections.onConnected("conn-1", session, sessions);
    connections.onConnected("conn-2", session, sessions);

    const auto binding = connections.sessionFor("conn-1");
    REQUIRE(binding.has_value());
    CHECK(binding->session == session);
    CHECK(binding->color == 'w');

    CHECK_FALSE(connections.sessionFor("unknown-conn").has_value());

    // connectionsFor returns the binding (including color) directly, so a
    // caller fanning a message out to a session's players never needs a
    // second sessionFor() lookup per recipient.
    const auto entries = connections.connectionsFor(session);
    REQUIRE(entries.size() == 2);
    for (const auto& [id, entryBinding] : entries) {
        CHECK(entryBinding.session == session);
        CHECK((entryBinding.color == 'w' || entryBinding.color == 'b'));
    }
}

TEST_CASE("ConnectionManager: a color reserved by PlayerSessionRegistry is not handed to a new connection") {
    // A disconnected player still within their reconnect window must keep
    // their seat - a brand-new (non-reconnect) connection attempting to
    // grab that color must be rejected, exactly like TABLE_FULL for an
    // ordinary third player.
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    connections.onConnected("conn-1", session, sessions);  // 'w', live
    const std::string token = sessions.registerNew("bob", 'b', session, "conn-2");
    connections.onDisconnected("conn-2");
    sessions.markDisconnected("conn-2", 1000);  // 'b' now reserved, not live

    const ConnectionOutcome thirdPartyAttempt = connections.onConnected("conn-3", session, sessions);

    CHECK_FALSE(thirdPartyAttempt.accepted);
}

TEST_CASE("ConnectionManager: bindKnown binds a connectionId directly, bypassing color assignment") {
    ConnectionManager connections;
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    connections.bindKnown("conn-reconnected", session, 'b');

    const auto binding = connections.sessionFor("conn-reconnected");
    REQUIRE(binding.has_value());
    CHECK(binding->session == session);
    CHECK(binding->color == 'b');
}
