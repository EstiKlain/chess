#include "doctest.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "client_net/domain_ports/IServerLink.hpp"
#include "client_net/application/ServerConnection.hpp"
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
        if (throwOnConnect) {
            throw std::runtime_error("simulated connect failure");
        }
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

    // Test helper: simulates the server pushing a message.
    void deliver(const std::string& rawJson) {
        if (onMessage) onMessage(rawJson);
    }

    // Test helper: simulates the link dying (a dropped connection).
    void simulateClose() {
        if (onClose) onClose();
    }

    std::string connectedHost;
    uint16_t connectedPort = 0;
    int connectCount = 0;
    bool throwOnConnect = false;
    std::vector<std::string> sent;
    OnMessageHandler onMessage;
    OnCloseHandler onClose;
    std::function<void(const std::string&)> onSend;
};

// Extracts the requestId a RECONNECT (or any) envelope was sent with, so
// tests can build a matching reply without hardcoding request-id counter
// values (which would make tests brittle against unrelated earlier sends).
std::string requestIdOf(const std::string& rawJson) {
    const auto key = rawJson.find("\"requestId\":\"");
    REQUIRE(key != std::string::npos);
    const auto start = key + std::string("\"requestId\":\"").size();
    const auto end = rawJson.find('"', start);
    return rawJson.substr(start, end - start);
}

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

    const bool result = connection.login("alice", "secret");

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

    const bool result = connection.login("", "secret");

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
        R"("players":[{"id":"conn-1","color":"w","name":"Alice","elo":1200}]}})");

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

TEST_CASE("ServerConnection: MATCH_FOUND's sessionToken is stored and readable via sessionToken()") {
    FakeServerLink link;
    ServerConnection connection(link);

    link.deliver(
        R"({"type":"MATCH_FOUND","requestId":"p1","payload":{)"
        R"("sessionToken":"abc123","color":"w","opponent":{"username":"bob","elo":1200}}})");

    const auto token = connection.sessionToken();
    REQUIRE(token.has_value());
    CHECK(*token == "abc123");
}

TEST_CASE("ServerConnection: sessionToken() is nullopt before any MATCH_FOUND is received") {
    FakeServerLink link;
    ServerConnection connection(link);

    CHECK_FALSE(connection.sessionToken().has_value());
}

TEST_CASE("ServerConnection: a delivered DISCONNECT_COUNTDOWN updates latestDisconnectCountdown()") {
    FakeServerLink link;
    ServerConnection connection(link);

    link.deliver(R"({"type":"DISCONNECT_COUNTDOWN","requestId":"","payload":{"secondsLeft":7}})");

    const auto seconds = connection.latestDisconnectCountdown();
    REQUIRE(seconds.has_value());
    CHECK(*seconds == 7);
}

TEST_CASE("ServerConnection: a DISCONNECT_COUNTDOWN with secondsLeft 0 clears the countdown back to nullopt") {
    FakeServerLink link;
    ServerConnection connection(link);

    link.deliver(R"({"type":"DISCONNECT_COUNTDOWN","requestId":"","payload":{"secondsLeft":7}})");
    REQUIRE(connection.latestDisconnectCountdown().has_value());

    link.deliver(R"({"type":"DISCONNECT_COUNTDOWN","requestId":"","payload":{"secondsLeft":0}})");

    CHECK_FALSE(connection.latestDisconnectCountdown().has_value());
}

TEST_CASE("ServerConnection: a malformed DISCONNECT_COUNTDOWN payload is ignored, not thrown") {
    FakeServerLink link;
    ServerConnection connection(link);

    link.deliver(R"({"type":"DISCONNECT_COUNTDOWN","requestId":"","payload":{"secondsLeft":"not-a-number"}})");

    CHECK_FALSE(connection.latestDisconnectCountdown().has_value());
}

TEST_CASE("ServerConnection: a STATE_UPDATE with gameOver true clears any in-effect disconnect countdown") {
    FakeServerLink link;
    ServerConnection connection(link);

    link.deliver(R"({"type":"DISCONNECT_COUNTDOWN","requestId":"","payload":{"secondsLeft":5}})");
    REQUIRE(connection.latestDisconnectCountdown().has_value());

    link.deliver(
        R"({"type":"STATE_UPDATE","requestId":"","payload":{)"
        R"("rows":8,"cols":8,"pieces":[],"gameOver":true,"nowMs":1,"role":"w","players":[],)"
        R"("winner":"b","reason":"resignation"}})");

    CHECK_FALSE(connection.latestDisconnectCountdown().has_value());
}

TEST_CASE("ServerConnection: myColor() is '\\0' before any STATE_UPDATE arrives") {
    FakeServerLink link;
    ServerConnection connection(link);

    CHECK(connection.myColor() == '\0');
}

TEST_CASE("ServerConnection: myColor() reflects the most recent STATE_UPDATE's per-recipient role") {
    FakeServerLink link;
    ServerConnection connection(link);

    link.deliver(
        R"({"type":"STATE_UPDATE","requestId":"","payload":{)"
        R"("rows":8,"cols":8,"pieces":[],"gameOver":false,"nowMs":1,"role":"b","players":[]}})");

    CHECK(connection.myColor() == 'b');
}

TEST_CASE("ServerConnection: requestPlay sends PLAY and returns immediately, in Searching state") {
    FakeServerLink link;
    ServerConnection connection(link);

    connection.requestPlay();

    REQUIRE(link.sent.size() == 1);
    CHECK(link.sent[0].find("\"type\":\"PLAY\"") != std::string::npos);
    CHECK(connection.matchmakingState() == MatchmakingState::Searching);
}

TEST_CASE("ServerConnection: a delivered MATCH_FOUND moves state to Matched and stores the opponent's name") {
    FakeServerLink link;
    ServerConnection connection(link);
    connection.requestPlay();

    link.deliver(
        R"({"type":"MATCH_FOUND","requestId":"p1","payload":{)"
        R"("sessionToken":"tok-1","color":"b","opponent":{"username":"carol","elo":1300}}})");

    CHECK(connection.matchmakingState() == MatchmakingState::Matched);
    REQUIRE(connection.matchedOpponentName().has_value());
    CHECK(*connection.matchedOpponentName() == "carol");
}

TEST_CASE("ServerConnection: a NO_MATCH_FOUND ERROR moves state to NoMatch") {
    FakeServerLink link;
    ServerConnection connection(link);
    connection.requestPlay();

    link.deliver(
        R"({"type":"ERROR","requestId":"p1","payload":{)"
        R"("code":"NO_MATCH_FOUND","message":"no opponent found within the matchmaking window"}})");

    CHECK(connection.matchmakingState() == MatchmakingState::NoMatch);
}

TEST_CASE("ServerConnection: an ALREADY_IN_GAME ERROR moves state to AlreadyInGame, not left stuck in Searching") {
    FakeServerLink link;
    ServerConnection connection(link);
    connection.requestPlay();

    link.deliver(R"({"type":"ERROR","requestId":"p1","payload":{"code":"ALREADY_IN_GAME","message":"already in a game"}})");

    CHECK(connection.matchmakingState() == MatchmakingState::AlreadyInGame);
}

TEST_CASE("ServerConnection: reconnect() without ever having logged in returns Retry, sends nothing") {
    FakeServerLink link;
    ServerConnection connection(link);

    const auto outcome = connection.reconnect();

    CHECK(outcome == ReconnectOutcome::Retry);
    CHECK(link.sent.empty());
}

TEST_CASE("ServerConnection: reconnect() succeeds when a matching STATE_UPDATE arrives") {
    FakeServerLink link;
    ServerConnection connection(link);
    link.onSend = [&](const std::string& rawJson) {
        if (rawJson.find("\"type\":\"LOGIN\"") != std::string::npos) {
            link.deliver(R"({"type":"LOGIN_OK","requestId":")" + requestIdOf(rawJson) +
                         R"(","payload":{"username":"alice","elo":1200}})");
            link.deliver(
                R"({"type":"MATCH_FOUND","requestId":"","payload":{)"
                R"("sessionToken":"tok-1","color":"w","opponent":{"username":"bob","elo":1200}}})");
        } else if (rawJson.find("\"type\":\"RECONNECT\"") != std::string::npos) {
            link.deliver(
                R"({"type":"STATE_UPDATE","requestId":")" + requestIdOf(rawJson) +
                R"(","payload":{"rows":8,"cols":8,"pieces":[],"gameOver":false,"nowMs":1,"role":"w","players":[]}})");
        }
    };
    connection.connect("host", 1);
    REQUIRE(connection.login("alice", "secret"));
    link.connectCount = 0;  // isolate the reconnect's own redial from login's initial connect()

    const auto outcome = connection.reconnect();

    CHECK(outcome == ReconnectOutcome::Success);
    CHECK(link.connectCount == 1);
}

TEST_CASE("ServerConnection: reconnect() returns Retry when the link fails to redial") {
    FakeServerLink link;
    ServerConnection connection(link);
    link.onSend = [&](const std::string& rawJson) {
        if (rawJson.find("\"type\":\"LOGIN\"") != std::string::npos) {
            link.deliver(R"({"type":"LOGIN_OK","requestId":")" + requestIdOf(rawJson) +
                         R"(","payload":{"username":"alice","elo":1200}})");
            link.deliver(
                R"({"type":"MATCH_FOUND","requestId":"","payload":{)"
                R"("sessionToken":"tok-1","color":"w","opponent":{"username":"bob","elo":1200}}})");
        }
    };
    REQUIRE(connection.login("alice", "secret"));
    link.throwOnConnect = true;
    link.sent.clear();  // discard the earlier LOGIN send, isolate reconnect()'s own behavior

    const auto outcome = connection.reconnect();

    CHECK(outcome == ReconnectOutcome::Retry);
    CHECK(link.sent.empty());  // never got far enough to send RECONNECT
}

TEST_CASE("ServerConnection: reconnect() returns Retry (not a permanent failure) on a SESSION_EXPIRED reply") {
    // Deliberately not a terminal outcome - see ReconnectOutcome's own doc
    // comment for why a single SESSION_EXPIRED reply can be a false
    // positive from a race with the server's own disconnect detection.
    FakeServerLink link;
    ServerConnection connection(link);
    link.onSend = [&](const std::string& rawJson) {
        if (rawJson.find("\"type\":\"LOGIN\"") != std::string::npos) {
            link.deliver(R"({"type":"LOGIN_OK","requestId":")" + requestIdOf(rawJson) +
                         R"(","payload":{"username":"alice","elo":1200}})");
            link.deliver(
                R"({"type":"MATCH_FOUND","requestId":"","payload":{)"
                R"("sessionToken":"tok-1","color":"w","opponent":{"username":"bob","elo":1200}}})");
        } else if (rawJson.find("\"type\":\"RECONNECT\"") != std::string::npos) {
            link.deliver(R"({"type":"ERROR","requestId":")" + requestIdOf(rawJson) +
                         R"(","payload":{"code":"SESSION_EXPIRED","message":"no game to reconnect to"}})");
        }
    };
    REQUIRE(connection.login("alice", "secret"));

    const auto outcome = connection.reconnect();

    CHECK(outcome == ReconnectOutcome::Retry);
}
