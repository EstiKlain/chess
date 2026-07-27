#include "doctest.h"

#include <string>
#include <utility>
#include <vector>

#include "engine/GameEngine.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/ConnectionManager.hpp"
#include "server/application/DisconnectUseCase.hpp"
#include "server/application/GameSession.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/config.hpp"
#include "server/domain_ports/IClock.hpp"
#include "server/domain_ports/ITransport.hpp"

namespace {

class FakeClock : public IClock {
public:
    long nowMs() const override { return nowMs_; }
    void advanceMs(long ms) { nowMs_ += ms; }

private:
    long nowMs_ = 0;
};

class FakeTransport : public ITransport {
public:
    void send(const std::string& connectionId, const std::string& rawJson) override {
        sent.emplace_back(connectionId, rawJson);
    }
    void broadcast(const std::string&) override {}
    void setOnOpen(OnOpenHandler) override {}
    void setOnClose(OnCloseHandler) override {}
    void setOnMessage(OnMessageHandler) override {}
    void run(uint16_t) override {}
    void stop() override {}

    std::vector<std::pair<std::string, std::string>> sent;
};

class FakeTokenGenerator : public ITokenGenerator {
public:
    std::string generate() override { return "token-" + std::to_string(++counter_); }

private:
    int counter_ = 0;
};

Board makeBoard() {
    RawBoard raw;
    raw.push_back({"wR", ".", ".", "."});
    raw.push_back({"bR", ".", ".", "."});
    return buildBoard(raw);
}

pieceRules::PieceRulesRegistry registry;

int countDisconnectCountdownSends(const FakeTransport& transport) {
    int count = 0;
    for (const auto& [id, rawJson] : transport.sent) {
        if (rawJson.find("DISCONNECT_COUNTDOWN") != std::string::npos) ++count;
    }
    return count;
}

}  // namespace

TEST_CASE("DisconnectUseCase: onDisconnected for an unbound connection is a no-op") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    FakeTransport transport;
    FakeClock clock;
    DisconnectUseCase useCase(sessions, connections, transport, clock);

    useCase.onDisconnected("never-registered");
    useCase.tick(0);

    CHECK(transport.sent.empty());
}

TEST_CASE("DisconnectUseCase: DISCONNECT_COUNTDOWN is sent to the opponent only on integer-second changes") {
    GameSession session(GameEngine(makeBoard(), registry));
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);
    const std::string token = sessions.registerNew("alice", 'w', &session, "white-conn");

    FakeTransport transport;
    FakeClock clock;
    DisconnectUseCase useCase(sessions, connections, transport, clock);

    useCase.onDisconnected("white-conn");
    connections.onDisconnected("white-conn");

    // Advance in small steps across the whole window - only ~20 sends
    // should happen (once per second), not one per 50ms tick.
    for (int i = 0; i < 400; ++i) {
        clock.advanceMs(50);
        useCase.tick(clock.nowMs());
    }

    const int sends = countDisconnectCountdownSends(transport);
    CHECK(sends >= 19);
    CHECK(sends <= 21);

    // Every DISCONNECT_COUNTDOWN went to the opponent, never the
    // disconnected player's own (now-gone) connection.
    for (const auto& [id, rawJson] : transport.sent) {
        if (rawJson.find("DISCONNECT_COUNTDOWN") != std::string::npos) {
            CHECK(id == "black-conn");
        }
    }
}

TEST_CASE("DisconnectUseCase: reconnecting before the window elapses cancels the resign") {
    GameSession session(GameEngine(makeBoard(), registry));
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);
    const std::string token = sessions.registerNew("alice", 'w', &session, "white-conn");

    FakeTransport transport;
    FakeClock clock;
    DisconnectUseCase useCase(sessions, connections, transport, clock);

    useCase.onDisconnected("white-conn");
    connections.onDisconnected("white-conn");

    clock.advanceMs(19000);
    useCase.tick(clock.nowMs());

    REQUIRE(sessions.reconnect(token, "white-conn-2"));
    connections.bindKnown("white-conn-2", &session, 'w');

    clock.advanceMs(2000);  // past the original 20s window
    useCase.tick(clock.nowMs());

    CHECK_FALSE(session.snapshot().gameOver);
}

TEST_CASE("DisconnectUseCase: no reconnect within the window resigns the seat automatically") {
    GameSession session(GameEngine(makeBoard(), registry));
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);
    const std::string token = sessions.registerNew("alice", 'w', &session, "white-conn");

    FakeTransport transport;
    FakeClock clock;
    DisconnectUseCase useCase(sessions, connections, transport, clock);

    useCase.onDisconnected("white-conn");
    connections.onDisconnected("white-conn");

    clock.advanceMs(server_config::kReconnectWindowMs);
    useCase.tick(clock.nowMs());

    const GameSnapshot snap = session.snapshot();
    CHECK(snap.gameOver);
    REQUIRE(snap.winner.has_value());
    CHECK(*snap.winner == 'b');
    REQUIRE(snap.gameOverReason.has_value());
    CHECK(*snap.gameOverReason == GameOverReason::Resignation);

    // The seat is freed for good - no reconnect using the old token works anymore.
    CHECK_FALSE(sessions.reconnect(token, "white-conn-3"));
}
