#include "doctest.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "server/application/LoginUseCase.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/IPasswordHasher.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/domain_ports/IUserRepository.hpp"

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

// In-memory, unsynchronized (no concurrency in a single-threaded test) fake
// of IUserRepository - createUser calls are trivially observable via count.
class FakeUserRepository : public IUserRepository {
public:
    std::optional<UserRecord> findByUsername(const std::string& username) override {
        const auto it = users_.find(username);
        if (it == users_.end()) return std::nullopt;
        return it->second;
    }
    void createUser(const std::string& username, const std::string& passwordHash) override {
        ++createUserCalls;
        users_[username] = UserRecord{username, passwordHash, 1200};
    }
    void updateElo(const std::string& username, int newElo) override {
        const auto it = users_.find(username);
        if (it != users_.end()) it->second.elo = newElo;
    }

    int createUserCalls = 0;

private:
    std::unordered_map<std::string, UserRecord> users_;
};

// "Hashing" is just tagging the plaintext, enough to distinguish
// right/wrong passwords without pulling libsodium into this test target.
class FakePasswordHasher : public IPasswordHasher {
public:
    std::string hash(const std::string& plainPassword) override { return "hashed:" + plainPassword; }
    bool verify(const std::string& plainPassword, const std::string& hash) override {
        return hash == "hashed:" + plainPassword;
    }
};

}  // namespace

TEST_CASE("LoginUseCase: a brand-new username registers automatically and LOGIN_OK is sent") {
    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    FakePasswordHasher hasher;
    LoginUseCase useCase(identities, transport, users, hasher);

    useCase.handleLogin("conn-1", "r1", nlohmann::json{{"username", "alice"}, {"password", "secret"}});

    CHECK(users.createUserCalls == 1);
    CHECK(identities.isLoggedIn("conn-1"));
    REQUIRE(identities.usernameFor("conn-1").has_value());
    CHECK(identities.usernameFor("conn-1").value() == "alice");

    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].first == "conn-1");
    CHECK(transport.sent[0].second.find("LOGIN_OK") != std::string::npos);
    CHECK(transport.sent[0].second.find("\"requestId\":\"r1\"") != std::string::npos);
}

TEST_CASE("LoginUseCase: LOGIN_OK carries the username and current Elo, no sessionToken") {
    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    FakePasswordHasher hasher;
    LoginUseCase useCase(identities, transport, users, hasher);

    useCase.handleLogin("conn-1", "r1", nlohmann::json{{"username", "alice"}, {"password", "secret"}});

    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("\"username\":\"alice\"") != std::string::npos);
    CHECK(transport.sent[0].second.find("\"elo\":1200") != std::string::npos);
    // No seat exists yet at LOGIN time - the token is minted only once PLAY matches this player (MatchStarter).
    CHECK(transport.sent[0].second.find("sessionToken") == std::string::npos);
}

TEST_CASE("LoginUseCase: an existing username with the right password logs in without re-registering") {
    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    FakePasswordHasher hasher;
    LoginUseCase useCase(identities, transport, users, hasher);

    useCase.handleLogin("conn-1", "r1", nlohmann::json{{"username", "alice"}, {"password", "secret"}});
    useCase.handleLogin("conn-2", "r2", nlohmann::json{{"username", "alice"}, {"password", "secret"}});

    CHECK(users.createUserCalls == 1);
    REQUIRE(transport.sent.size() == 2);
    CHECK(transport.sent[1].second.find("LOGIN_OK") != std::string::npos);
}

TEST_CASE("LoginUseCase: an existing username with the wrong password is rejected with INVALID_CREDENTIALS") {
    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    FakePasswordHasher hasher;
    LoginUseCase useCase(identities, transport, users, hasher);

    useCase.handleLogin("conn-1", "r1", nlohmann::json{{"username", "alice"}, {"password", "secret"}});
    useCase.handleLogin("conn-2", "r2", nlohmann::json{{"username", "alice"}, {"password", "wrong"}});

    CHECK_FALSE(identities.isLoggedIn("conn-2"));
    REQUIRE(transport.sent.size() == 2);
    CHECK(transport.sent[1].second.find("INVALID_CREDENTIALS") != std::string::npos);
}

TEST_CASE("LoginUseCase: an empty username is rejected with MALFORMED_PAYLOAD and no login is recorded") {
    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    FakePasswordHasher hasher;
    LoginUseCase useCase(identities, transport, users, hasher);

    useCase.handleLogin("conn-1", "r2", nlohmann::json{{"username", ""}, {"password", "secret"}});

    CHECK_FALSE(identities.isLoggedIn("conn-1"));
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("MALFORMED_PAYLOAD") != std::string::npos);
}

TEST_CASE("LoginUseCase: an empty password is rejected with MALFORMED_PAYLOAD") {
    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    FakePasswordHasher hasher;
    LoginUseCase useCase(identities, transport, users, hasher);

    useCase.handleLogin("conn-1", "r2", nlohmann::json{{"username", "alice"}, {"password", ""}});

    CHECK_FALSE(identities.isLoggedIn("conn-1"));
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("MALFORMED_PAYLOAD") != std::string::npos);
}

TEST_CASE("LoginUseCase: a payload missing the username field is rejected with MALFORMED_PAYLOAD") {
    FakeIdentityStore identities;
    FakeTransport transport;
    FakeUserRepository users;
    FakePasswordHasher hasher;
    LoginUseCase useCase(identities, transport, users, hasher);

    useCase.handleLogin("conn-1", "r3", nlohmann::json::object());

    CHECK_FALSE(identities.isLoggedIn("conn-1"));
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("MALFORMED_PAYLOAD") != std::string::npos);
}
