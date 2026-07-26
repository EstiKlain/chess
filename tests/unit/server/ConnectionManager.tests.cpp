#include "doctest.h"

#include "server/application/ConnectionManager.hpp"

// GameSession is only ever used here as an opaque pointer/key - forward
// declared in ConnectionManager.hpp, and ConnectionManager never dereferences
// it. No real GameSession construction (and therefore no GameEngine) needed
// for these tests.
class GameSession;

TEST_CASE("ConnectionManager: first connection gets 'w', second gets 'b'") {
    ConnectionManager connections;
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    const ConnectionOutcome first = connections.onConnected("conn-1", session);
    const ConnectionOutcome second = connections.onConnected("conn-2", session);

    CHECK(first.accepted);
    CHECK(first.color == 'w');
    CHECK(second.accepted);
    CHECK(second.color == 'b');
    CHECK(connections.connectionCount() == 2);
}

TEST_CASE("ConnectionManager: a third connection is rejected once both colors are taken") {
    ConnectionManager connections;
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    connections.onConnected("conn-1", session);
    connections.onConnected("conn-2", session);
    const ConnectionOutcome third = connections.onConnected("conn-3", session);

    CHECK_FALSE(third.accepted);
    CHECK(connections.connectionCount() == 2);
}

TEST_CASE("ConnectionManager: disconnecting frees a color for the next connection") {
    ConnectionManager connections;
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    connections.onConnected("conn-1", session);
    connections.onConnected("conn-2", session);
    connections.onDisconnected("conn-1");
    const ConnectionOutcome rejoin = connections.onConnected("conn-3", session);

    CHECK(rejoin.accepted);
    CHECK(rejoin.color == 'w');
}

TEST_CASE("ConnectionManager: sessionFor and connectionsFor reflect the bindings made") {
    ConnectionManager connections;
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    connections.onConnected("conn-1", session);
    connections.onConnected("conn-2", session);

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
