#include "doctest.h"

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "client_net/infrastructure/ClientLogger.hpp"

namespace {

class FakeServerLink : public IServerLink {
public:
    void connect(const std::string& host, uint16_t port) override {
        connectedHost = host;
        connectedPort = port;
    }
    void send(const std::string& rawJson) override { sent.push_back(rawJson); }
    void setOnMessage(OnMessageHandler handler) override { onMessage = std::move(handler); }
    void stop() override { stopped = true; }

    std::string connectedHost;
    uint16_t connectedPort = 0;
    std::vector<std::string> sent;
    OnMessageHandler onMessage;
    bool stopped = false;
};

class FakeLogger : public ILogger {
public:
    void log(LogDirection direction, const std::string& connectionId, const std::string& rawJson) override {
        calls.emplace_back(direction == LogDirection::Sent ? "SENT" : "RECEIVED", connectionId, rawJson);
    }

    std::vector<std::tuple<std::string, std::string, std::string>> calls;
};

}  // namespace

TEST_CASE("ClientLogger: connect() passes through untouched, with no logging") {
    FakeServerLink real;
    FakeLogger logger;
    ClientLogger link(real, logger);

    link.connect("localhost", 9002);

    CHECK(real.connectedHost == "localhost");
    CHECK(real.connectedPort == 9002);
    CHECK(logger.calls.empty());
}

TEST_CASE("ClientLogger: send() logs SENT with an empty connectionId, then delegates to the real link") {
    FakeServerLink real;
    FakeLogger logger;
    ClientLogger link(real, logger);

    link.send("payload");

    REQUIRE(real.sent.size() == 1);
    CHECK(real.sent[0] == "payload");

    REQUIRE(logger.calls.size() == 1);
    CHECK(std::get<0>(logger.calls[0]) == "SENT");
    CHECK(std::get<1>(logger.calls[0]) == "");
    CHECK(std::get<2>(logger.calls[0]) == "payload");
}

TEST_CASE("ClientLogger: an incoming message logs RECEIVED before invoking the original handler, unchanged") {
    FakeServerLink real;
    FakeLogger logger;
    ClientLogger link(real, logger);

    std::vector<std::string> handlerCalls;
    link.setOnMessage([&](const std::string& rawJson) { handlerCalls.push_back(rawJson); });

    REQUIRE(real.onMessage);
    real.onMessage("incoming");

    REQUIRE(logger.calls.size() == 1);
    CHECK(std::get<0>(logger.calls[0]) == "RECEIVED");
    CHECK(std::get<1>(logger.calls[0]) == "");
    CHECK(std::get<2>(logger.calls[0]) == "incoming");

    REQUIRE(handlerCalls.size() == 1);
    CHECK(handlerCalls[0] == "incoming");
}

TEST_CASE("ClientLogger: stop() passes through untouched, with no logging") {
    FakeServerLink real;
    FakeLogger logger;
    ClientLogger link(real, logger);

    link.stop();

    CHECK(real.stopped);
    CHECK(logger.calls.empty());
}
