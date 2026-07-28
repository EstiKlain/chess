#include "doctest.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "server/application/ConnectionManager.hpp"
#include "server/application/LoginUseCase.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

namespace {

// Fake IIdentityStore local to this file, matching the codebase's existing
// per-test-file fake convention (no shared test-fakes header exists yet).
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

}  // namespace

TEST_CASE("LoginUseCase: a non-empty username is accepted and LOGIN_OK is sent to the sender") {
    FakeIdentityStore identities;
    FakeTransport transport;
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("conn-1", nullptr, sessions);
    LoginUseCase useCase(identities, transport, connections, sessions);

    useCase.handleLogin("conn-1", "r1", nlohmann::json{{"username", "alice"}});

    CHECK(identities.isLoggedIn("conn-1"));
    REQUIRE(identities.usernameFor("conn-1").has_value());
    CHECK(identities.usernameFor("conn-1").value() == "alice");

    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].first == "conn-1");
    CHECK(transport.sent[0].second.find("LOGIN_OK") != std::string::npos);
    CHECK(transport.sent[0].second.find("\"requestId\":\"r1\"") != std::string::npos);
}

TEST_CASE("LoginUseCase: LOGIN_OK carries a sessionToken usable for a later RECONNECT") {
    FakeIdentityStore identities;
    FakeTransport transport;
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("conn-1", nullptr, sessions);
    LoginUseCase useCase(identities, transport, connections, sessions);

    useCase.handleLogin("conn-1", "r1", nlohmann::json{{"username", "alice"}});

    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("\"sessionToken\"") != std::string::npos);
}

TEST_CASE("LoginUseCase: an empty username is rejected with MALFORMED_PAYLOAD and no login is recorded") {
    FakeIdentityStore identities;
    FakeTransport transport;
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("conn-1", nullptr, sessions);
    LoginUseCase useCase(identities, transport, connections, sessions);

    useCase.handleLogin("conn-1", "r2", nlohmann::json{{"username", ""}});

    CHECK_FALSE(identities.isLoggedIn("conn-1"));
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("MALFORMED_PAYLOAD") != std::string::npos);
}

TEST_CASE("LoginUseCase: a payload missing the username field is rejected with MALFORMED_PAYLOAD") {
    FakeIdentityStore identities;
    FakeTransport transport;
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("conn-1", nullptr, sessions);
    LoginUseCase useCase(identities, transport, connections, sessions);

    useCase.handleLogin("conn-1", "r3", nlohmann::json::object());

    CHECK_FALSE(identities.isLoggedIn("conn-1"));
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("MALFORMED_PAYLOAD") != std::string::npos);
}

TEST_CASE("LoginUseCase: logging in again on the same connection replaces the previous username") {
    FakeIdentityStore identities;
    FakeTransport transport;
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("conn-1", nullptr, sessions);
    LoginUseCase useCase(identities, transport, connections, sessions);

    useCase.handleLogin("conn-1", "r4", nlohmann::json{{"username", "alice"}});
    useCase.handleLogin("conn-1", "r5", nlohmann::json{{"username", "bob"}});

    REQUIRE(identities.usernameFor("conn-1").has_value());
    CHECK(identities.usernameFor("conn-1").value() == "bob");
}

TEST_CASE("LoginUseCase: a connection with no session binding is rejected with TABLE_FULL, not LOGIN_OK") {
    FakeIdentityStore identities;
    FakeTransport transport;
    ConnectionManager connections;  // "conn-1" deliberately never bound - e.g. a 3rd/rejected connection.
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    LoginUseCase useCase(identities, transport, connections, sessions);

    useCase.handleLogin("conn-1", "r6", nlohmann::json{{"username", "alice"}});

    CHECK_FALSE(identities.isLoggedIn("conn-1"));
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].first == "conn-1");
    CHECK(transport.sent[0].second.find("TABLE_FULL") != std::string::npos);
    CHECK(transport.sent[0].second.find("LOGIN_OK") == std::string::npos);
}
