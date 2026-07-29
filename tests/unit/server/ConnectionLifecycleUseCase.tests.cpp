#include "doctest.h"

#include <optional>
#include <string>
#include <unordered_map>

#include "engine/GameEngine.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/ConnectionLifecycleUseCase.hpp"
#include "server/application/ConnectionManager.hpp"
#include "server/application/DisconnectUseCase.hpp"
#include "server/application/GameSession.hpp"
#include "server/application/PlayRequestUseCase.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
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
    void send(const std::string&, const std::string&) override {}
    void broadcast(const std::string&) override {}
    void setOnOpen(OnOpenHandler) override {}
    void setOnClose(OnCloseHandler) override {}
    void setOnMessage(OnMessageHandler) override {}
    void run(uint16_t) override {}
    void stop() override {}
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
    std::optional<UserRecord> findByUsername(const std::string&) override { return std::nullopt; }
    void createUser(const std::string&, const std::string&) override {}
    void updateElo(const std::string&, int) override {}
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

struct Fixture {
    ConnectionManager connections;
    FakeIdentityStore identities;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions{tokens};
    FakeTransport transport;
    FakeClock clock;
    DisconnectUseCase disconnectUseCase{sessions, connections, transport, clock};
    InMemoryMatchmakingQueue queue;
    FakeUserRepository users;
    PlayRequestUseCase playRequestUseCase{queue,  connections, sessions, identities, users, transport, clock,
                                           [](const WaitingPlayer&, const WaitingPlayer&) {}};
    ConnectionLifecycleUseCase lifecycle{connections, identities, disconnectUseCase, playRequestUseCase};
};

}  // namespace

TEST_CASE("ConnectionLifecycleUseCase: disconnecting a queued player removes them from the matchmaking queue") {
    Fixture f;
    f.identities.login("conn-alice", "alice");
    f.playRequestUseCase.handlePlay("conn-alice", "req-1", nlohmann::json::object());
    REQUIRE(f.queue.contains("conn-alice"));

    f.lifecycle.onDisconnected("conn-alice");

    CHECK_FALSE(f.queue.contains("conn-alice"));
    CHECK_FALSE(f.identities.usernameFor("conn-alice").has_value());
}

TEST_CASE("ConnectionLifecycleUseCase: disconnecting a player mid-game opens the reconnect window, not the queue") {
    Fixture f;
    GameSession session(GameEngine(makeBoard(), registry));
    f.connections.bindKnown("conn-alice", &session, 'w');
    const std::string token = f.sessions.registerNew("alice", 'w', &session, "conn-alice");
    f.identities.login("conn-alice", "alice");

    f.lifecycle.onDisconnected("conn-alice");

    CHECK_FALSE(f.connections.sessionFor("conn-alice").has_value());
    CHECK_FALSE(f.identities.usernameFor("conn-alice").has_value());
    CHECK_FALSE(f.queue.contains("conn-alice"));  // never entered it - was already in a game

    const auto entry = f.sessions.find(token);
    REQUIRE(entry.has_value());
    CHECK_FALSE(entry->connectionId.has_value());  // marked disconnected
    CHECK(entry->disconnectedAtMs.has_value());
}

TEST_CASE("ConnectionLifecycleUseCase: disconnecting a connection that was never bound or queued is a no-op") {
    Fixture f;

    f.lifecycle.onDisconnected("never-registered");  // must not throw

    CHECK_FALSE(f.queue.contains("never-registered"));
}
