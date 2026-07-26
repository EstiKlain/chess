#include "doctest.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "client_net/IServerLink.hpp"
#include "client_net/ServerConnection.hpp"
#include "engine/MoveRequest.hpp"
#include "model/Position.hpp"

namespace {

// Fake IServerLink local to this file, matching the codebase's existing
// per-test-file fake convention. send() is synchronous, so tests that need
// a "server reply" set onSend to deliver one immediately, in-line - this
// keeps everything on one thread with no real concurrency, the same way
// the rest of this project's use-case tests fake their ports.
class FakeServerLink : public IServerLink {
public:
    void connect(const std::string& host, uint16_t port) override {
        connectedHost = host;
        connectedPort = port;
    }
    void send(const std::string& rawJson) override {
        sent.push_back(rawJson);
        if (onSend) {
            onSend(rawJson);
        }
    }
    void setOnMessage(OnMessageHandler handler) override { onMessage = std::move(handler); }
    void stop() override {}

    // Test helper: simulates the server pushing a message.
    void deliver(const std::string& rawJson) {
        if (onMessage) onMessage(rawJson);
    }

    std::string connectedHost;
    uint16_t connectedPort = 0;
    std::vector<std::string> sent;
    OnMessageHandler onMessage;
    std::function<void(const std::string&)> onSend;
};

}  // namespace

TEST_CASE("ServerConnection: connect() forwards host/port to the link") {
    FakeServerLink link;
    ServerConnection connection(link);

    connection.connect("example.com", 1234);

    CHECK(link.connectedHost == "example.com");
    CHECK(link.connectedPort == 1234);
}

TEST_CASE("ServerConnection: login() sends LOGIN and resolves true on LOGIN_OK") {
    FakeServerLink link;
    ServerConnection connection(link);
    link.onSend = [&](const std::string&) {
        link.deliver(R"({"type":"LOGIN_OK","requestId":"1","payload":{}})");
    };

    const bool result = connection.login("alice");

    CHECK(result);
    REQUIRE(link.sent.size() == 1);
    CHECK(link.sent[0].find("\"type\":\"LOGIN\"") != std::string::npos);
    CHECK(link.sent[0].find("alice") != std::string::npos);
}

TEST_CASE("ServerConnection: login() resolves false and records lastError on ERROR") {
    FakeServerLink link;
    ServerConnection connection(link);
    link.onSend = [&](const std::string&) {
        link.deliver(
            R"({"type":"ERROR","requestId":"1","payload":{"code":"MALFORMED_PAYLOAD","message":"username must not be empty"}})");
    };

    const bool result = connection.login("");

    CHECK_FALSE(result);
    CHECK(connection.lastError() == "username must not be empty");
}

TEST_CASE("ServerConnection: requestMove sends a MOVE with the domain MoveRequest's coordinates") {
    FakeServerLink link;
    ServerConnection connection(link);

    connection.requestMove(MoveRequest{Position{1, 2}, Position{3, 4}});

    REQUIRE(link.sent.size() == 1);
    CHECK(link.sent[0].find("\"type\":\"MOVE\"") != std::string::npos);
    CHECK(link.sent[0].find("\"fromRow\":1") != std::string::npos);
    CHECK(link.sent[0].find("\"fromCol\":2") != std::string::npos);
    CHECK(link.sent[0].find("\"toRow\":3") != std::string::npos);
    CHECK(link.sent[0].find("\"toCol\":4") != std::string::npos);
}

TEST_CASE("ServerConnection: requestJump sends a JUMP with row/col, not MOVE's shape") {
    FakeServerLink link;
    ServerConnection connection(link);

    connection.requestJump(5, 6);

    REQUIRE(link.sent.size() == 1);
    CHECK(link.sent[0].find("\"type\":\"JUMP\"") != std::string::npos);
    CHECK(link.sent[0].find("\"row\":5") != std::string::npos);
    CHECK(link.sent[0].find("\"col\":6") != std::string::npos);
}

TEST_CASE("ServerConnection: a delivered STATE_UPDATE updates latestSnapshot() and latestPlayers()") {
    FakeServerLink link;
    ServerConnection connection(link);

    link.deliver(
        R"({"type":"STATE_UPDATE","requestId":"","payload":{)"
        R"("rows":8,"cols":8,)"
        R"("pieces":[{"id":1,"color":"w","kind":"R","row":0,"col":0,"state":"Idle","stateStartMs":0,"stateDurationMs":0}],)"
        R"("gameOver":false,"nowMs":123,"role":"w",)"
        R"("players":[{"id":"conn-1","color":"w","name":"Alice"}]}})");

    const GameSnapshot snapshot = connection.latestSnapshot();
    CHECK(snapshot.rows == 8);
    CHECK(snapshot.cols == 8);
    REQUIRE(snapshot.pieces.size() == 1);
    CHECK(snapshot.pieces[0].kind == 'R');
    CHECK(snapshot.pieces[0].color == 'w');

    const auto players = connection.latestPlayers();
    REQUIRE(players.size() == 1);
    CHECK(players[0].name == "Alice");
    CHECK(players[0].color == "w");
}

TEST_CASE("ServerConnection: awaitInitialSnapshot returns once a STATE_UPDATE has already been delivered") {
    FakeServerLink link;
    ServerConnection connection(link);

    link.deliver(
        R"({"type":"STATE_UPDATE","requestId":"","payload":{)"
        R"("rows":8,"cols":8,"pieces":[],"gameOver":false,"nowMs":1,"role":"w","players":[]}})");

    const GameSnapshot snapshot = connection.awaitInitialSnapshot();
    CHECK(snapshot.rows == 8);
    CHECK(snapshot.cols == 8);
}
