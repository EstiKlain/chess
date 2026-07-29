#include "doctest.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/GameEngine.hpp"
#include "engine/MoveRequest.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/ConnectionManager.hpp"
#include "server/application/DisconnectUseCase.hpp"
#include "server/application/EloUpdateUseCase.hpp"
#include "server/application/GameSessionStore.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/application/ServerTick.hpp"
#include "server/domain_ports/IClock.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/domain_ports/IUserRepository.hpp"
#include "shared/protocol/config.hpp"

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

class FakeIdentityStore : public IIdentityStore {
public:
    void login(const std::string& connectionId, const std::string& username) override {
        names_[connectionId] = username;
    }
    bool isLoggedIn(const std::string& connectionId) const override { return names_.count(connectionId) > 0; }
    std::optional<std::string> usernameFor(const std::string& connectionId) const override {
        const auto it = names_.find(connectionId);
        if (it == names_.end()) return std::nullopt;
        return it->second;
    }
    void logout(const std::string& connectionId) override { names_.erase(connectionId); }

private:
    std::unordered_map<std::string, std::string> names_;
};

class FakeUserRepository : public IUserRepository {
public:
    std::optional<UserRecord> findByUsername(const std::string& username) override {
        const auto it = users_.find(username);
        if (it == users_.end()) return std::nullopt;
        return it->second;
    }
    void createUser(const std::string& username, const std::string& passwordHash) override {
        users_[username] = UserRecord{username, passwordHash, server_config::kInitialElo};
    }
    void updateElo(const std::string& username, int newElo) override {
        const auto it = users_.find(username);
        if (it != users_.end()) it->second.elo = newElo;
    }
    void seed(const std::string& username, int elo) { users_[username] = UserRecord{username, "hash", elo}; }

private:
    std::unordered_map<std::string, UserRecord> users_;
};

class FakeTokenGenerator : public ITokenGenerator {
public:
    std::string generate() override { return "token-" + std::to_string(++counter_); }

private:
    int counter_ = 0;
};

pieceRules::PieceRulesRegistry registry;

Board twoKingsBoard() {
    RawBoard raw{{"wK", "."}, {".", "bK"}};
    return buildBoard(raw);
}

GameEngine twoKingsEngine() { return GameEngine(twoKingsBoard(), registry); }

int countStateUpdates(const FakeTransport& transport) {
    int count = 0;
    for (const auto& [id, rawJson] : transport.sent) {
        if (rawJson.find("STATE_UPDATE") != std::string::npos) ++count;
    }
    return count;
}

}  // namespace

TEST_CASE("ServerTick: advances and broadcasts to every session, independently") {
    GameSessionStore store(twoKingsEngine);
    GameSession& sessionA = store.create(true);
    GameSession& sessionB = store.create(true);

    ConnectionManager connections;
    connections.bindKnown("a-w", &sessionA, 'w');
    connections.bindKnown("a-b", &sessionA, 'b');
    connections.bindKnown("b-w", &sessionB, 'w');
    connections.bindKnown("b-b", &sessionB, 'b');

    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    FakeTransport transport;
    FakeClock clock;
    FakeIdentityStore identities;
    DisconnectUseCase disconnectUseCase(sessions, connections, transport, clock);
    FakeUserRepository users;
    EloUpdateUseCase eloUpdateUseCase(users, sessions, connections);

    ServerTick serverTick(store, disconnectUseCase, eloUpdateUseCase, connections, identities, transport, clock);
    serverTick.tick(50);

    CHECK(countStateUpdates(transport) == 4);  // 2 recipients per session, 2 sessions
    CHECK_FALSE(sessionA.snapshot().gameOver);
    CHECK_FALSE(sessionB.snapshot().gameOver);
}

TEST_CASE("ServerTick: a disconnect-timeout resignation and its Elo update land in the same tick") {
    GameSessionStore store(twoKingsEngine);
    GameSession& session = store.create(true);

    ConnectionManager connections;
    connections.bindKnown("conn-b", &session, 'b');  // bob stays connected; alice (white) does not

    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    sessions.registerNew("alice", 'w', &session, "conn-w");
    sessions.registerNew("bob", 'b', &session, "conn-b");
    sessions.markDisconnected("conn-w", 0);  // alice disconnected at t=0

    FakeTransport transport;
    FakeClock clock;
    clock.advanceMs(server_config::kReconnectWindowMs + 1000);  // past the window - this tick must resign her

    FakeIdentityStore identities;
    identities.login("conn-b", "bob");
    DisconnectUseCase disconnectUseCase(sessions, connections, transport, clock);
    FakeUserRepository users;
    users.seed("alice", 1200);
    users.seed("bob", 1200);
    EloUpdateUseCase eloUpdateUseCase(users, sessions, connections);

    ServerTick serverTick(store, disconnectUseCase, eloUpdateUseCase, connections, identities, transport, clock);
    serverTick.tick(50);

    const GameSnapshot snap = session.snapshot();
    REQUIRE(snap.gameOver);
    REQUIRE(snap.winner.has_value());
    CHECK(*snap.winner == 'b');  // alice (white) resigned by timeout

    const auto bobRecord = users.findByUsername("bob");
    REQUIRE(bobRecord.has_value());
    CHECK(bobRecord->elo > 1200);  // Elo already applied in this same tick() call
}

TEST_CASE("ServerTick: a same-tick king capture wins over a same-tick disconnect-timeout resignation") {
    GameSessionStore store([]() {
        RawBoard raw{{"wR", ".", "bK"}};
        return GameEngine(buildBoard(raw), registry);
    });
    GameSession& session = store.create(true);
    REQUIRE(session.requestMove(MoveRequest{Position{0, 0}, Position{0, 2}}).accepted);  // rook captures black king

    ConnectionManager connections;
    connections.bindKnown("conn-b", &session, 'b');

    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    sessions.registerNew("alice", 'w', &session, "conn-w");
    sessions.registerNew("bob", 'b', &session, "conn-b");
    // alice (white) is also timing out this same tick. If the disconnect-timeout resigned her before the
    // in-flight capture resolved, black would "win" by resignation - the wrong outcome.
    sessions.markDisconnected("conn-w", 0);

    FakeTransport transport;
    FakeClock clock;
    clock.advanceMs(server_config::kReconnectWindowMs + 1000);

    FakeIdentityStore identities;
    DisconnectUseCase disconnectUseCase(sessions, connections, transport, clock);
    FakeUserRepository users;
    EloUpdateUseCase eloUpdateUseCase(users, sessions, connections);

    ServerTick serverTick(store, disconnectUseCase, eloUpdateUseCase, connections, identities, transport, clock);
    serverTick.tick(2000);  // long enough for the rook's move to resolve

    const GameSnapshot snap = session.snapshot();
    REQUIRE(snap.gameOver);
    REQUIRE(snap.winner.has_value());
    CHECK(*snap.winner == 'w');  // the king capture, not the disconnect-resign
}
