#include "doctest.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/GameEngine.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/ConnectionManager.hpp"
#include "server/application/GameSession.hpp"
#include "server/application/MakeMoveUseCase.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

namespace {

// Same fakes as MessageRouter.tests.cpp: real core, fake ports - per the
// plan's test doctrine, never a real socket/bus implementation in a unit test.
class FakeEventBus : public IEventBus {
public:
    void subscribe(const std::string&, EventHandler) override {}
    void publish(const BusEvent& event) override { published.push_back(event); }
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

class FakeTokenGenerator : public ITokenGenerator {
public:
    std::string generate() override { return "token-" + std::to_string(++counter_); }

private:
    int counter_ = 0;
};

Board makeBoard(std::initializer_list<std::initializer_list<std::string>> rows) {
    RawBoard raw;
    for (const auto& row : rows) {
        raw.push_back(std::vector<std::string>(row.begin(), row.end()));
    }
    return buildBoard(raw);
}

pieceRules::PieceRulesRegistry registry;

}  // namespace

TEST_CASE("MakeMoveUseCase: a legal MOVE publishes MoveApplied and sends STATE_UPDATE to both players") {
    GameSession session(GameEngine(makeBoard({{"wR", ".", ".", "."}, {"bR", ".", ".", "."}}), registry));
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);

    FakeEventBus bus;
    FakeTransport transport;
    FakeIdentityStore identities;
    identities.login("white-conn", "Alice");
    identities.login("black-conn", "Bob");
    MakeMoveUseCase useCase(bus, transport, connections, identities);

    useCase.handleMove("white-conn", "r1",
                        nlohmann::json{{"fromRow", 0}, {"fromCol", 0}, {"toRow", 0}, {"toCol", 3}});

    REQUIRE(bus.published.size() == 1);
    CHECK(bus.published[0].type == "MoveApplied");

    REQUIRE(transport.sent.size() == 2);
    for (const auto& [connectionId, rawJson] : transport.sent) {
        CHECK(rawJson.find("STATE_UPDATE") != std::string::npos);
    }
    // Each recipient's STATE_UPDATE carries their OWN role - never the same
    // shared payload sent to both.
    const std::string& whitePayload = transport.sent[0].first == "white-conn" ? transport.sent[0].second : transport.sent[1].second;
    const std::string& blackPayload = transport.sent[0].first == "black-conn" ? transport.sent[0].second : transport.sent[1].second;
    CHECK(whitePayload.find("\"role\":\"w\"") != std::string::npos);
    CHECK(blackPayload.find("\"role\":\"b\"") != std::string::npos);

    // requestId correlation: only the mover (white) gets "r1" echoed back -
    // black never sent this request, so its STATE_UPDATE carries "".
    CHECK(whitePayload.find("\"requestId\":\"r1\"") != std::string::npos);
    CHECK(blackPayload.find("\"requestId\":\"\"") != std::string::npos);

    // players[] is identical for both recipients and carries both names.
    CHECK(whitePayload.find("\"name\":\"Alice\"") != std::string::npos);
    CHECK(whitePayload.find("\"name\":\"Bob\"") != std::string::npos);
    CHECK(blackPayload.find("\"name\":\"Alice\"") != std::string::npos);
    CHECK(blackPayload.find("\"name\":\"Bob\"") != std::string::npos);
}

TEST_CASE("MakeMoveUseCase: an illegal MOVE sends ERROR/ILLEGAL_MOVE only to the sender") {
    GameSession session(GameEngine(makeBoard({{"wR", ".", ".", "."}, {"bR", ".", ".", "."}}), registry));
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);

    FakeEventBus bus;
    FakeTransport transport;
    FakeIdentityStore identities;
    identities.login("white-conn", "Alice");
    identities.login("black-conn", "Bob");
    MakeMoveUseCase useCase(bus, transport, connections, identities);

    // Off the board - guaranteed illegal regardless of piece-specific rules.
    useCase.handleMove("white-conn", "r2",
                        nlohmann::json{{"fromRow", 99}, {"fromCol", 99}, {"toRow", 0}, {"toCol", 0}});

    CHECK(bus.published.empty());
    REQUIRE(transport.sent.size() == 1);
    CHECK(transport.sent[0].first == "white-conn");
    CHECK(transport.sent[0].second.find("ILLEGAL_MOVE") != std::string::npos);
    CHECK(transport.sent[0].second.find("\"requestId\":\"r2\"") != std::string::npos);
}

TEST_CASE("MakeMoveUseCase: JUMP uses GameEngine::requestJump's single-position shape, not MOVE's") {
    GameSession session(GameEngine(makeBoard({{"wR", ".", ".", "."}}), registry));
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);

    FakeEventBus bus;
    FakeTransport transport;
    FakeIdentityStore identities;
    identities.login("white-conn", "Alice");
    identities.login("black-conn", "Bob");
    MakeMoveUseCase useCase(bus, transport, connections, identities);

    useCase.handleJump("white-conn", "r3", nlohmann::json{{"row", 0}, {"col", 0}});

    REQUIRE(bus.published.size() == 1);
    CHECK(bus.published[0].type == "MoveApplied");
    REQUIRE(transport.sent.size() == 2);
}
