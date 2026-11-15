#include "doctest.h"

#include "server/application/ConnectionManager.hpp"

// GameSession is only ever used here as an opaque pointer/key - forward
// declared in ConnectionManager.hpp, and ConnectionManager never dereferences
// it. No real GameSession construction (and therefore no GameEngine) needed
// for these tests.
class GameSession;

TEST_CASE("ConnectionManager: sessionFor and connectionsFor reflect the bindings made") {
    ConnectionManager connections;
    GameSession* session = reinterpret_cast<GameSession*>(0x1);

    connections.bindKnown("conn-1", session, 'w');
    connections.bindKnown("conn-2", session, 'b');

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

TEST_CASE("ConnectionManager: setElo updates the live Elo cache on an already-bound connection") {
    ConnectionManager connections;
    GameSession* session = reinterpret_cast<GameSession*>(0x1);
    connections.bindKnown("conn-1", session, 'w');

    REQUIRE(connections.sessionFor("conn-1").has_value());
    CHECK(connections.sessionFor("conn-1")->elo == server_config::kInitialElo);

    connections.setElo("conn-1", 1350);

    REQUIRE(connections.sessionFor("conn-1").has_value());
    CHECK(connections.sessionFor("conn-1")->elo == 1350);
}

TEST_CASE("ConnectionManager: setElo on an unbound connectionId is a no-op") {
    ConnectionManager connections;

    connections.setElo("unknown-conn", 1350);  // must not throw or create a binding

    CHECK_FALSE(connections.sessionFor("unknown-conn").has_value());
}

TEST_CASE("ConnectionManager: connectionsFor only returns the players of the session asked about, with two sessions live") {
    // Guards the secondary bySession_ index added for Server-Iteration 7:
    // connectionsFor must stay O(this session's players), not scan every
    // connection in the server - and must still filter correctly once more
    // than one GameSession exists at once.
    ConnectionManager connections;
    GameSession* sessionA = reinterpret_cast<GameSession*>(0x1);
    GameSession* sessionB = reinterpret_cast<GameSession*>(0x2);

    connections.bindKnown("a-1", sessionA, 'w');
    connections.bindKnown("a-2", sessionA, 'b');
    connections.bindKnown("b-1", sessionB, 'w');
    connections.bindKnown("b-2", sessionB, 'b');

    const auto forA = connections.connectionsFor(sessionA);
    const auto forB = connections.connectionsFor(sessionB);

    REQUIRE(forA.size() == 2);
    for (const auto& [id, binding] : forA) {
        CHECK(binding.session == sessionA);
    }
    REQUIRE(forB.size() == 2);
    for (const auto& [id, binding] : forB) {
        CHECK(binding.session == sessionB);
    }

    // Disconnecting one player of session A must not disturb session B's roster.
    connections.onDisconnected("a-1");
    CHECK(connections.connectionsFor(sessionA).size() == 1);
    CHECK(connections.connectionsFor(sessionB).size() == 2);
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
