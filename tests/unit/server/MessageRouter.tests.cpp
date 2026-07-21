#include "doctest.h"

#include <string>
#include <utility>
#include <vector>

#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/protocol/MessageRouter.hpp"

namespace {

// Fake IEventBus: records every published event, no real subscribers needed
// for these tests - we only assert on what got published.
class FakeEventBus : public IEventBus {
public:
    void subscribe(const std::string&, EventHandler) override {
        // Not needed for MessageRouter tests: the router only publishes,
        // it never subscribes.
    }

    void publish(const BusEvent& event) override { published.push_back(event); }

    std::vector<BusEvent> published;
};

// Fake ITransport: records every send() call, no real socket involved.
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

}  // namespace

TEST_CASE("MessageRouter: a valid message is published on the bus, not sent directly") {
    FakeEventBus bus;
    FakeTransport transport;
    MessageRouter router(bus, transport);

    router.handleRawMessage("conn-1", R"({"type":"PING","requestId":"r1","payload":{}})");

    REQUIRE(bus.published.size() == 1);
    CHECK(bus.published[0].type == "PING");
    CHECK(bus.published[0].connectionId == "conn-1");
    CHECK(transport.sent.empty());
}

TEST_CASE("MessageRouter: the payload is passed through to the bus event untouched") {
    FakeEventBus bus;
    FakeTransport transport;
    MessageRouter router(bus, transport);

    router.handleRawMessage(
        "conn-1", R"({"type":"MOVE","requestId":"r2","payload":{"from":"e2","to":"e4"}})");

    REQUIRE(bus.published.size() == 1);
    CHECK(bus.published[0].payload.at("from") == "e2");
    CHECK(bus.published[0].payload.at("to") == "e4");
}

TEST_CASE("MessageRouter: malformed JSON triggers an immediate ERROR reply, not a bus event") {
    FakeEventBus bus;
    FakeTransport transport;
    MessageRouter router(bus, transport);

    router.handleRawMessage("conn-1", "{not valid json");

    CHECK(bus.published.empty());
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].first == "conn-1");
    CHECK(transport.sent[0].second.find("ERROR") != std::string::npos);
}

TEST_CASE("MessageRouter: a message missing the required 'type' field is treated as malformed") {
    FakeEventBus bus;
    FakeTransport transport;
    MessageRouter router(bus, transport);

    router.handleRawMessage("conn-1", R"({"requestId":"r3","payload":{}})");

    CHECK(bus.published.empty());
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].second.find("ERROR") != std::string::npos);
}
