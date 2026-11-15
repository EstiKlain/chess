#include "doctest.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/GameEngine.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/ConnectionManager.hpp"
#include "server/application/GameSessionStore.hpp"
#include "server/application/MatchStarter.hpp"
#include "server/application/PlayRequestUseCase.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/config.hpp"
#include "server/domain_ports/IClock.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/domain_ports/IUserRepository.hpp"
#include "server/infrastructure/matchmaking/InMemoryMatchmakingQueue.hpp"

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

Board makeBoard() {
    RawBoard raw{{"wK", "."}, {".", "bK"}};
    return buildBoard(raw);
}

GameEngine makeEngine() { return GameEngine(makeBoard(), registry); }

int countMessages(const FakeTransport& transport, const std::string& type) {
    int count = 0;
    for (const auto& [id, rawJson] : transport.sent) {
        if (rawJson.find("\"" + type + "\"") != std::string::npos) ++count;
    }
    return count;
}

struct Fixture {
    InMemoryMatchmakingQueue queue;
    GameSessionStore store{makeEngine};
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions{tokens};
    FakeIdentityStore identities;
    FakeUserRepository users;
    FakeTransport transport;
    FakeClock clock;
    // Wires the same way main_server.cpp's composition root will: PlayRequestUseCase never sees
    // GameSessionStore directly, only this callback, which is what actually calls MatchStarter::start.
    PlayRequestUseCase useCase{queue, connections, sessions, identities, users, transport, clock,
                                [this](const WaitingPlayer& waiting, const WaitingPlayer& joining) {
                                    MatchStarter::start(store, connections, sessions, transport, waiting, joining);
                                }};

    void login(const std::string& connectionId, const std::string& username, int elo) {
        identities.login(connectionId, username);
        users.seed(username, elo);
    }
};

}  // namespace

TEST_CASE("PlayRequestUseCase: PLAY with nobody waiting enqueues and sends no reply") {
    Fixture f;
    f.login("conn-alice", "alice", 1200);

    f.useCase.handlePlay("conn-alice", "req-1", nlohmann::json::object());

    CHECK(f.transport.sent.empty());
    CHECK(f.queue.contains("conn-alice"));
}

TEST_CASE("PlayRequestUseCase: two players within range match immediately, oldest gets white") {
    Fixture f;
    f.login("conn-alice", "alice", 1200);
    f.login("conn-bob", "bob", 1250);

    f.useCase.handlePlay("conn-alice", "req-alice", nlohmann::json::object());
    f.useCase.handlePlay("conn-bob", "req-bob", nlohmann::json::object());

    REQUIRE(f.store.allSessions().size() == 1);
    CHECK(countMessages(f.transport, "MATCH_FOUND") == 2);
    CHECK_FALSE(f.queue.contains("conn-alice"));
    CHECK_FALSE(f.queue.contains("conn-bob"));

    const auto aliceBinding = f.connections.sessionFor("conn-alice");
    const auto bobBinding = f.connections.sessionFor("conn-bob");
    REQUIRE(aliceBinding.has_value());
    REQUIRE(bobBinding.has_value());
    CHECK(aliceBinding->color == 'w');  // alice queued first
    CHECK(bobBinding->color == 'b');

    // Each player's own requestId, not the other's, is echoed back.
    bool aliceGotOwnRequestId = false;
    bool bobGotOwnRequestId = false;
    for (const auto& [id, rawJson] : f.transport.sent) {
        if (id == "conn-alice" && rawJson.find("\"req-alice\"") != std::string::npos) aliceGotOwnRequestId = true;
        if (id == "conn-bob" && rawJson.find("\"req-bob\"") != std::string::npos) bobGotOwnRequestId = true;
    }
    CHECK(aliceGotOwnRequestId);
    CHECK(bobGotOwnRequestId);
}

TEST_CASE("PlayRequestUseCase: players outside Elo range both wait, no match") {
    Fixture f;
    f.login("conn-alice", "alice", 1200);
    f.login("conn-bob", "bob", 1500);

    f.useCase.handlePlay("conn-alice", "req-alice", nlohmann::json::object());
    f.useCase.handlePlay("conn-bob", "req-bob", nlohmann::json::object());

    CHECK(f.store.allSessions().empty());
    CHECK(f.transport.sent.empty());
    CHECK(f.queue.contains("conn-alice"));
    CHECK(f.queue.contains("conn-bob"));
}

TEST_CASE("PlayRequestUseCase: tick sends NO_MATCH_FOUND only after the matchmaking timeout") {
    Fixture f;
    f.login("conn-alice", "alice", 1200);
    f.useCase.handlePlay("conn-alice", "req-1", nlohmann::json::object());

    f.clock.advanceMs(59000);
    f.useCase.tick(f.clock.nowMs());
    CHECK(f.transport.sent.empty());
    CHECK(f.queue.contains("conn-alice"));

    f.clock.advanceMs(2000);  // now at 61s
    f.useCase.tick(f.clock.nowMs());

    REQUIRE(f.transport.sent.size() == 1);
    CHECK(f.transport.sent[0].first == "conn-alice");
    CHECK(f.transport.sent[0].second.find("NO_MATCH_FOUND") != std::string::npos);
    CHECK(f.transport.sent[0].second.find("\"req-1\"") != std::string::npos);
    CHECK_FALSE(f.queue.contains("conn-alice"));
}

TEST_CASE("PlayRequestUseCase: a second PLAY from a connection already searching is rejected") {
    Fixture f;
    f.login("conn-alice", "alice", 1200);
    f.useCase.handlePlay("conn-alice", "req-1", nlohmann::json::object());

    f.useCase.handlePlay("conn-alice", "req-2", nlohmann::json::object());

    REQUIRE(f.transport.sent.size() == 1);
    CHECK(f.transport.sent[0].second.find("ALREADY_SEARCHING") != std::string::npos);
}

TEST_CASE("PlayRequestUseCase: PLAY from a connection already bound to a game is rejected") {
    Fixture f;
    f.connections.bindKnown("conn-alice", &f.store.create(true), 'w');
    f.login("conn-alice", "alice", 1200);

    f.useCase.handlePlay("conn-alice", "req-1", nlohmann::json::object());

    REQUIRE(f.transport.sent.size() == 1);
    CHECK(f.transport.sent[0].second.find("ALREADY_IN_GAME") != std::string::npos);
    CHECK_FALSE(f.queue.contains("conn-alice"));
}

TEST_CASE("PlayRequestUseCase: onDisconnected removes a queued player so a later PLAY cannot match them") {
    Fixture f;
    f.login("conn-alice", "alice", 1200);
    f.useCase.handlePlay("conn-alice", "req-1", nlohmann::json::object());
    REQUIRE(f.queue.contains("conn-alice"));

    f.useCase.onDisconnected("conn-alice");
    CHECK_FALSE(f.queue.contains("conn-alice"));

    f.login("conn-bob", "bob", 1200);
    f.useCase.handlePlay("conn-bob", "req-bob", nlohmann::json::object());

    CHECK(f.transport.sent.empty());  // no ghost match with the departed alice
    CHECK(f.store.allSessions().empty());
}

TEST_CASE(
    "PlayRequestUseCase: a disconnected player still inside their reconnect window cannot be matched into a second "
    "game") {
    Fixture f;
    GameSession& firstGame = f.store.create(true);
    f.sessions.registerNew("alice", 'w', &firstGame, "old-conn");
    // alice's connection dropped, but DisconnectUseCase hasn't resigned her yet - the entry is still registered and
    // firstGame is still running (not gameOver). A fresh connection under the same username must not be matched
    // into a second, unrelated game while that seat is still reserved.
    f.login("conn-alice-2", "alice", 1200);

    f.useCase.handlePlay("conn-alice-2", "req-1", nlohmann::json::object());

    REQUIRE(f.transport.sent.size() == 1);
    CHECK(f.transport.sent[0].second.find("ALREADY_IN_GAME") != std::string::npos);
    CHECK(f.store.allSessions().size() == 1);  // no second session was created
}

TEST_CASE("PlayRequestUseCase: a username whose previous game has ended can be matched into a new one") {
    Fixture f;
    GameSession& finishedGame = f.store.create(true);
    finishedGame.resign('b');  // game over: white (alice) won
    f.sessions.registerNew("alice", 'w', &finishedGame, "old-conn");
    f.login("conn-alice-2", "alice", 1200);
    f.login("conn-bob", "bob", 1200);

    f.useCase.handlePlay("conn-alice-2", "req-alice", nlohmann::json::object());
    f.useCase.handlePlay("conn-bob", "req-bob", nlohmann::json::object());

    CHECK(f.store.allSessions().size() == 2);  // finishedGame + the new match
    CHECK(countMessages(f.transport, "MATCH_FOUND") == 2);
}
