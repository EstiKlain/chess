#include "doctest.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "client_net/application/AutoReconnector.hpp"
#include "client_net/application/ServerConnection.hpp"
#include "client_net/domain_ports/IServerLink.hpp"

namespace {

// Real ServerConnection + a fake port, per this project's test doctrine -
// AutoReconnector's tests exercise the real application logic, never a real
// socket. Same fake shape as ServerConnection.tests.cpp's own.
class FakeServerLink : public IServerLink {
public:
    void connect(const std::string& host, uint16_t port) override {
        connectedHost = host;
        connectedPort = port;
        ++connectCount;
    }
    void send(const std::string& rawJson) override {
        sent.push_back(rawJson);
        if (onSend) {
            onSend(rawJson);
        }
    }
    void setOnMessage(OnMessageHandler handler) override { onMessage = std::move(handler); }
    void setOnClose(OnCloseHandler handler) override { onClose = std::move(handler); }
    void stop() override {}

    void deliver(const std::string& rawJson) {
        if (onMessage) onMessage(rawJson);
    }
    void simulateClose() {
        if (onClose) onClose();
    }

    std::string connectedHost;
    uint16_t connectedPort = 0;
    std::atomic<int> connectCount{0};
    std::vector<std::string> sent;
    OnMessageHandler onMessage;
    OnCloseHandler onClose;
    std::function<void(const std::string&)> onSend;
};

std::string requestIdOf(const std::string& rawJson) {
    const auto key = rawJson.find("\"requestId\":\"");
    REQUIRE(key != std::string::npos);
    const auto start = key + std::string("\"requestId\":\"").size();
    const auto end = rawJson.find('"', start);
    return rawJson.substr(start, end - start);
}

// AutoReconnector's work happens on a background thread - these tests poll
// with a bounded timeout rather than asserting instantaneously, the usual
// pattern for observing background-thread state from a test.
template <typename Predicate>
bool waitUntil(Predicate predicate, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
}

}  // namespace

TEST_CASE("AutoReconnector: a simulated close with no prior login does nothing") {
    FakeServerLink link;
    ServerConnection connection(link);
    AutoReconnector reconnector(connection);

    link.simulateClose();

    CHECK_FALSE(waitUntil([&] { return reconnector.isReconnecting(); }, std::chrono::milliseconds(200)));
    CHECK_FALSE(reconnector.hasFailed());
}

TEST_CASE("AutoReconnector: a close after login triggers retries until a matching STATE_UPDATE succeeds") {
    FakeServerLink link;
    ServerConnection connection(link);
    link.onSend = [&](const std::string& rawJson) {
        if (rawJson.find("\"type\":\"LOGIN\"") != std::string::npos) {
            link.deliver(R"({"type":"LOGIN_OK","requestId":")" + requestIdOf(rawJson) +
                         R"(","payload":{"sessionToken":"tok-1"}})");
        } else if (rawJson.find("\"type\":\"RECONNECT\"") != std::string::npos) {
            link.deliver(
                R"({"type":"STATE_UPDATE","requestId":")" + requestIdOf(rawJson) +
                R"(","payload":{"rows":8,"cols":8,"pieces":[],"gameOver":false,"nowMs":1,"role":"w","players":[]}})");
        }
    };
    REQUIRE(connection.login("alice"));
    AutoReconnector reconnector(connection);

    link.simulateClose();

    REQUIRE(waitUntil([&] { return !reconnector.isReconnecting(); }));
    CHECK_FALSE(reconnector.hasFailed());
}

TEST_CASE("AutoReconnector: repeated SESSION_EXPIRED replies keep retrying, not an immediate failure") {
    FakeServerLink link;
    ServerConnection connection(link);
    link.onSend = [&](const std::string& rawJson) {
        if (rawJson.find("\"type\":\"LOGIN\"") != std::string::npos) {
            link.deliver(R"({"type":"LOGIN_OK","requestId":")" + requestIdOf(rawJson) +
                         R"(","payload":{"sessionToken":"tok-1"}})");
        } else if (rawJson.find("\"type\":\"RECONNECT\"") != std::string::npos) {
            link.deliver(R"({"type":"ERROR","requestId":")" + requestIdOf(rawJson) +
                         R"(","payload":{"code":"SESSION_EXPIRED","message":"no game to reconnect to"}})");
        }
    };
    REQUIRE(connection.login("alice"));
    AutoReconnector reconnector(connection);

    link.simulateClose();

    // Wait for at least two attempts (each SESSION_EXPIRED resolves near-
    // instantly here, so attempts are paced by AutoReconnector's own
    // kRetryIntervalMs between them) - it must still be actively retrying,
    // not have given up after the first SESSION_EXPIRED (see
    // ReconnectOutcome's doc comment for why a single reply is never
    // treated as terminal).
    REQUIRE(waitUntil([&] { return link.connectCount >= 2; }, std::chrono::milliseconds(3000)));
    CHECK(reconnector.isReconnecting());
    CHECK_FALSE(reconnector.hasFailed());
}
