#include "doctest.h"

#include "server/infrastructure/bus/InProcessEventBus.hpp"

TEST_CASE("InProcessEventBus: a subscriber receives an event of its own type") {
    InProcessEventBus bus;
    int callCount = 0;
    BusEvent received;

    bus.subscribe("PING", [&](const BusEvent& event) {
        ++callCount;
        received = event;
    });

    bus.publish(BusEvent{"PING", "conn-1", "r1", nlohmann::json{{"hello", "world"}}});

    CHECK(callCount == 1);
    CHECK(received.type == "PING");
    CHECK(received.connectionId == "conn-1");
    CHECK(received.requestId == "r1");
    CHECK(received.payload.at("hello") == "world");
}

TEST_CASE("InProcessEventBus: a subscriber on a different type is never called") {
    InProcessEventBus bus;
    int pingCalls = 0;
    int moveCalls = 0;

    bus.subscribe("PING", [&](const BusEvent&) { ++pingCalls; });
    bus.subscribe("MOVE", [&](const BusEvent&) { ++moveCalls; });

    bus.publish(BusEvent{"PING", "conn-1", "", nlohmann::json::object()});

    CHECK(pingCalls == 1);
    CHECK(moveCalls == 0);
}

TEST_CASE("InProcessEventBus: multiple subscribers on the same type all get called") {
    InProcessEventBus bus;
    int firstCalls = 0;
    int secondCalls = 0;

    bus.subscribe("PING", [&](const BusEvent&) { ++firstCalls; });
    bus.subscribe("PING", [&](const BusEvent&) { ++secondCalls; });

    bus.publish(BusEvent{"PING", "conn-1", "", nlohmann::json::object()});

    CHECK(firstCalls == 1);
    CHECK(secondCalls == 1);
}

TEST_CASE("InProcessEventBus: publishing with no subscribers is a silent no-op") {
    InProcessEventBus bus;
    CHECK_NOTHROW(bus.publish(BusEvent{"NOBODY_LISTENS", "conn-1", "", nlohmann::json::object()}));
}
