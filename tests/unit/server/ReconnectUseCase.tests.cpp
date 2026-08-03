#include "doctest.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/GameEngine.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/ConnectionManager.hpp"
#include "server/application/GameSession.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/application/ReconnectUseCase.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/domain_ports/IUserRepository.hpp"

namespace {

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

class FakeTokenGenerator : public ITokenGenerator {
public:
    std::string generate() override { return "token-" + std::to_string(++counter_); }

private:
    int counter_ = 0;
};

// Local fake, matching this codebase's per-test-file fake convention (see
// LoginUseCase.tests.cpp for the same shape).
class FakeUserRepository : public IUserRepository {
public:
    std::optional<UserRecord> findByUsername(const std::string& username) override {
        const auto it = users_.find(username);
        if (it == users_.end()) return std::nullopt;
        return it->second;
    }
    void createUser(const std::string& username, const std::string& passwordHash) override {
        users_[username] = UserRecord{username, passwordHash, 1200};
    }
    void updateElo(const std::string& username, int newElo) override {
        const auto it = users_.find(username);
        if (it != users_.end()) it->second.elo = newElo;
    }

private:
    std::unordered_map<std::string, UserRecord> users_;
};

Board makeBoard() {
    RawBoard raw;
    raw.push_back({"wR", ".", ".", "."});
    raw.push_back({"bR", ".", ".", "."});
    return buildBoard(raw);
}

pieceRules::PieceRulesRegistry registry;

}  // namespace

TEST_CASE("ReconnectUseCase: a valid, still-disconnected token re-binds the connection and sends STATE_UPDATE") {
    GameSession session(GameEngine(makeBoard(), registry));
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    connections.bindKnown("white-conn", &session, 'w');
    connections.bindKnown("black-conn", &session, 'b');
    const std::string token = sessions.registerNew("alice", 'w', &session, "white-conn");
    connections.onDisconnected("white-conn");
    sessions.markDisconnected("white-conn", 1000);

    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    users.createUser("alice", "irrelevant-hash");
    users.updateElo("alice", 1350);
    ReconnectUseCase useCase(sessions, connections, identities, transport, users);

    useCase.handleReconnect("white-conn-2", "r1", nlohmann::json{{"sessionToken", token}});

    REQUIRE(connections.sessionFor("white-conn-2").has_value());
    CHECK(connections.sessionFor("white-conn-2")->elo == 1350);

    CHECK(identities.isLoggedIn("white-conn-2"));
    REQUIRE(identities.usernameFor("white-conn-2").has_value());
    CHECK(*identities.usernameFor("white-conn-2") == "alice");

    const auto binding = connections.sessionFor("white-conn-2");
    REQUIRE(binding.has_value());
    CHECK(binding->session == &session);
    CHECK(binding->color == 'w');

    // Three messages: StateFanOut::broadcast sends a STATE_UPDATE to every
    // connection bound to the session (the reconnecting player and the
    // opponent), plus ReconnectUseCase's own opponent-only "countdown is
    // over" cancel message - located by connectionId + content, not fixed
    // index, since black-conn now receives two independent sends.
    REQUIRE(transport.sent.size() == 3);
    auto findSentToContaining = [&](const std::string& connectionId, const std::string& needle) -> const std::string& {
        for (const auto& [id, rawJson] : transport.sent) {
            if (id == connectionId && rawJson.find(needle) != std::string::npos) return rawJson;
        }
        FAIL("no message found for connectionId " << connectionId << " containing " << needle);
        static const std::string empty;
        return empty;
    };

    const std::string& reconnectedPayload = findSentToContaining("white-conn-2", "STATE_UPDATE");
    CHECK(reconnectedPayload.find("\"requestId\":\"r1\"") != std::string::npos);
    CHECK(reconnectedPayload.find("\"role\":\"w\"") != std::string::npos);

    const std::string& opponentStateUpdate = findSentToContaining("black-conn", "STATE_UPDATE");
    CHECK(opponentStateUpdate.find("\"requestId\":\"\"") != std::string::npos);
    CHECK(opponentStateUpdate.find("\"role\":\"b\"") != std::string::npos);

    const std::string& opponentCountdown = findSentToContaining("black-conn", "DISCONNECT_COUNTDOWN");
    CHECK(opponentCountdown.find("\"secondsLeft\":0") != std::string::npos);
}

TEST_CASE("ReconnectUseCase: a token for an already-resigned session gets SESSION_EXPIRED, even before the registry entry is physically removed") {
    // DisconnectUseCase now defers removing a resigned token's registry
    // entry by one tick (so EloUpdateUseCase can still read it in the same
    // server tick) - this proves the "can't rejoin a finished game"
    // guarantee holds regardless of that removal timing, via the gameOver
    // check here rather than the entry's mere existence.
    GameSession session(GameEngine(makeBoard(), registry));
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    const std::string token = sessions.registerNew("alice", 'w', &session, "white-conn");
    sessions.markDisconnected("white-conn", 0);
    REQUIRE(session.resign('w'));  // simulates DisconnectUseCase's timeout resignation

    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    ReconnectUseCase useCase(sessions, connections, identities, transport, users);

    useCase.handleReconnect("white-conn-2", "r1", nlohmann::json{{"sessionToken", token}});

    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("SESSION_EXPIRED") != std::string::npos);
    CHECK_FALSE(connections.sessionFor("white-conn-2").has_value());
}

TEST_CASE("ReconnectUseCase: an unknown token gets SESSION_EXPIRED") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    ReconnectUseCase useCase(sessions, connections, identities, transport, users);

    useCase.handleReconnect("some-conn", "r1", nlohmann::json{{"sessionToken", "nonexistent"}});

    CHECK_FALSE(identities.isLoggedIn("some-conn"));
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("SESSION_EXPIRED") != std::string::npos);
}

TEST_CASE("ReconnectUseCase: a token that is no longer disconnected (already reconnected) gets SESSION_EXPIRED") {
    GameSession session(GameEngine(makeBoard(), registry));
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    connections.bindKnown("white-conn", &session, 'w');
    const std::string token = sessions.registerNew("alice", 'w', &session, "white-conn");
    connections.onDisconnected("white-conn");
    sessions.markDisconnected("white-conn", 1000);

    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    ReconnectUseCase useCase(sessions, connections, identities, transport, users);

    useCase.handleReconnect("white-conn-2", "r1", nlohmann::json{{"sessionToken", token}});
    transport.sent.clear();

    useCase.handleReconnect("white-conn-3", "r2", nlohmann::json{{"sessionToken", token}});

    CHECK_FALSE(identities.isLoggedIn("white-conn-3"));
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("SESSION_EXPIRED") != std::string::npos);
}

TEST_CASE("ReconnectUseCase: a malformed payload (missing sessionToken) gets MALFORMED_PAYLOAD") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    ReconnectUseCase useCase(sessions, connections, identities, transport, users);

    useCase.handleReconnect("some-conn", "r1", nlohmann::json::object());

    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("MALFORMED_PAYLOAD") != std::string::npos);
}
