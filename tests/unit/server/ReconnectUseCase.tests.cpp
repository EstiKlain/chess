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
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);
    const std::string token = sessions.registerNew("alice", 'w', &session, "white-conn");
    connections.onDisconnected("white-conn");
    sessions.markDisconnected("white-conn", 1000);

    FakeIdentityStore identities;
    FakeTransport transport;
    ReconnectUseCase useCase(sessions, connections, identities, transport);

    useCase.handleReconnect("white-conn-2", "r1", nlohmann::json{{"sessionToken", token}});

    CHECK(identities.isLoggedIn("white-conn-2"));
    REQUIRE(identities.usernameFor("white-conn-2").has_value());
    CHECK(*identities.usernameFor("white-conn-2") == "alice");

    const auto binding = connections.sessionFor("white-conn-2");
    REQUIRE(binding.has_value());
    CHECK(binding->session == &session);
    CHECK(binding->color == 'w');

    // Two messages: the reconnecting player's own STATE_UPDATE, plus the
    // opponent's "countdown is over" cancel message - located by
    // connectionId, not fixed index, since the two are independent sends.
    REQUIRE(transport.sent.size() == 2);
    auto findSentTo = [&](const std::string& connectionId) -> const std::string& {
        for (const auto& [id, rawJson] : transport.sent) {
            if (id == connectionId) return rawJson;
        }
        FAIL("no message found for connectionId " << connectionId);
        static const std::string empty;
        return empty;
    };

    const std::string& reconnectedPayload = findSentTo("white-conn-2");
    CHECK(reconnectedPayload.find("STATE_UPDATE") != std::string::npos);
    CHECK(reconnectedPayload.find("\"role\":\"w\"") != std::string::npos);

    const std::string& opponentPayload = findSentTo("black-conn");
    CHECK(opponentPayload.find("DISCONNECT_COUNTDOWN") != std::string::npos);
    CHECK(opponentPayload.find("\"secondsLeft\":0") != std::string::npos);
}

TEST_CASE("ReconnectUseCase: an unknown token gets SESSION_EXPIRED") {
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    ConnectionManager connections;
    FakeIdentityStore identities;
    FakeTransport transport;
    ReconnectUseCase useCase(sessions, connections, identities, transport);

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
    connections.onConnected("white-conn", &session, sessions);
    const std::string token = sessions.registerNew("alice", 'w', &session, "white-conn");
    connections.onDisconnected("white-conn");
    sessions.markDisconnected("white-conn", 1000);

    FakeIdentityStore identities;
    FakeTransport transport;
    ReconnectUseCase useCase(sessions, connections, identities, transport);

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
    ReconnectUseCase useCase(sessions, connections, identities, transport);

    useCase.handleReconnect("some-conn", "r1", nlohmann::json::object());

    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("MALFORMED_PAYLOAD") != std::string::npos);
}
