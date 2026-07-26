#include "doctest.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "server/application/AuthGuard.hpp"
#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

namespace {

class FakeEventBus : public IEventBus {
public:
    void subscribe(const std::string& eventType, EventHandler) override { subscribedTypes.push_back(eventType); }
    void publish(const BusEvent& event) override { published.push_back(event); }

    std::vector<std::string> subscribedTypes;
    std::vector<BusEvent> published;
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

}  // namespace

TEST_CASE("AuthGuard: LOGIN is forwarded to the wrapped bus even when not logged in") {
    FakeEventBus wrappedBus;
    FakeIdentityStore identities;
    FakeTransport transport;
    AuthGuard guard(wrappedBus, identities, transport);

    guard.publish(BusEvent{"LOGIN", "conn-1", "r1", nlohmann::json::object()});

    REQUIRE(wrappedBus.published.size() == 1);
    CHECK(wrappedBus.published[0].type == "LOGIN");
    CHECK(transport.sent.empty());
}

TEST_CASE("AuthGuard: PING is forwarded to the wrapped bus even when not logged in") {
    FakeEventBus wrappedBus;
    FakeIdentityStore identities;
    FakeTransport transport;
    AuthGuard guard(wrappedBus, identities, transport);

    guard.publish(BusEvent{"PING", "conn-1", "r1", nlohmann::json::object()});

    REQUIRE(wrappedBus.published.size() == 1);
    CHECK(transport.sent.empty());
}

TEST_CASE("AuthGuard: MOVE from a logged-in connection is forwarded to the wrapped bus") {
    FakeEventBus wrappedBus;
    FakeIdentityStore identities;
    FakeTransport transport;
    AuthGuard guard(wrappedBus, identities, transport);
    identities.login("conn-1", "alice");

    guard.publish(BusEvent{"MOVE", "conn-1", "r2", nlohmann::json::object()});

    REQUIRE(wrappedBus.published.size() == 1);
    CHECK(transport.sent.empty());
}

TEST_CASE("AuthGuard: MOVE from a connection that never logged in gets AUTH_REQUIRED directly, not forwarded") {
    FakeEventBus wrappedBus;
    FakeIdentityStore identities;
    FakeTransport transport;
    AuthGuard guard(wrappedBus, identities, transport);

    guard.publish(BusEvent{"MOVE", "conn-1", "r3", nlohmann::json::object()});

    CHECK(wrappedBus.published.empty());
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].first == "conn-1");
    CHECK(transport.sent[0].second.find("AUTH_REQUIRED") != std::string::npos);
    CHECK(transport.sent[0].second.find("\"requestId\":\"r3\"") != std::string::npos);
}

TEST_CASE("AuthGuard: subscribe() forwards to the wrapped bus unchanged") {
    FakeEventBus wrappedBus;
    FakeIdentityStore identities;
    FakeTransport transport;
    AuthGuard guard(wrappedBus, identities, transport);

    guard.subscribe("MOVE", [](const BusEvent&) {});

    REQUIRE(wrappedBus.subscribedTypes.size() == 1);
    CHECK(wrappedBus.subscribedTypes[0] == "MOVE");
}
