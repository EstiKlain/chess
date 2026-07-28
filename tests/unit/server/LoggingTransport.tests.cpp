#include "doctest.h"

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "server/infrastructure/transport/LoggingTransport.hpp"

namespace {

class FakeTransport : public ITransport {
public:
    void send(const std::string& connectionId, const std::string& rawJson) override {
        sent.emplace_back(connectionId, rawJson);
    }
    void broadcast(const std::string& rawJson) override { broadcasted.push_back(rawJson); }
    void setOnOpen(OnOpenHandler handler) override { onOpen = std::move(handler); }
    void setOnClose(OnCloseHandler handler) override { onClose = std::move(handler); }
    void setOnMessage(OnMessageHandler handler) override { onMessage = std::move(handler); }
    void run(uint16_t port) override { ranOnPort = port; }
    void stop() override { stopped = true; }

    std::vector<std::pair<std::string, std::string>> sent;
    std::vector<std::string> broadcasted;
    OnOpenHandler onOpen;
    OnCloseHandler onClose;
    OnMessageHandler onMessage;
    uint16_t ranOnPort = 0;
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

TEST_CASE("LoggingTransport: send() logs SENT with the connectionId, then delegates to the real transport") {
    FakeTransport real;
    FakeLogger logger;
    LoggingTransport transport(real, logger);

    transport.send("conn-1", "payload");

    REQUIRE(real.sent.size() == 1);
    CHECK(real.sent[0] == std::make_pair(std::string("conn-1"), std::string("payload")));

    REQUIRE(logger.calls.size() == 1);
    CHECK(std::get<0>(logger.calls[0]) == "SENT");
    CHECK(std::get<1>(logger.calls[0]) == "conn-1");
    CHECK(std::get<2>(logger.calls[0]) == "payload");
}

TEST_CASE("LoggingTransport: broadcast() logs SENT with an empty connectionId, then delegates") {
    FakeTransport real;
    FakeLogger logger;
    LoggingTransport transport(real, logger);

    transport.broadcast("payload");

    REQUIRE(real.broadcasted.size() == 1);
    CHECK(real.broadcasted[0] == "payload");

    REQUIRE(logger.calls.size() == 1);
    CHECK(std::get<0>(logger.calls[0]) == "SENT");
    CHECK(std::get<1>(logger.calls[0]) == "");
}

TEST_CASE("LoggingTransport: an incoming message logs RECEIVED before invoking the original handler, unchanged") {
    FakeTransport real;
    FakeLogger logger;
    LoggingTransport transport(real, logger);

    std::vector<std::pair<std::string, std::string>> handlerCalls;
    transport.setOnMessage([&](const std::string& connectionId, const std::string& rawJson) {
        handlerCalls.emplace_back(connectionId, rawJson);
    });

    REQUIRE(real.onMessage);
    real.onMessage("conn-2", "incoming");

    REQUIRE(logger.calls.size() == 1);
    CHECK(std::get<0>(logger.calls[0]) == "RECEIVED");
    CHECK(std::get<1>(logger.calls[0]) == "conn-2");
    CHECK(std::get<2>(logger.calls[0]) == "incoming");

    REQUIRE(handlerCalls.size() == 1);
    CHECK(handlerCalls[0] == std::make_pair(std::string("conn-2"), std::string("incoming")));
}

TEST_CASE("LoggingTransport: setOnOpen/setOnClose/run/stop pass through untouched, with no logging") {
    FakeTransport real;
    FakeLogger logger;
    LoggingTransport transport(real, logger);

    bool openCalled = false;
    bool closeCalled = false;
    transport.setOnOpen([&](const std::string&) { openCalled = true; });
    transport.setOnClose([&](const std::string&) { closeCalled = true; });

    REQUIRE(real.onOpen);
    REQUIRE(real.onClose);
    real.onOpen("conn-3");
    real.onClose("conn-3");
    CHECK(openCalled);
    CHECK(closeCalled);

    transport.run(9002);
    CHECK(real.ranOnPort == 9002);

    transport.stop();
    CHECK(real.stopped);

    CHECK(logger.calls.empty());
}
