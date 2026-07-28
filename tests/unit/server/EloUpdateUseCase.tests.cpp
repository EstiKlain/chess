#include "doctest.h"

#include <optional>
#include <string>
#include <unordered_map>

#include "engine/GameEngine.hpp"
#include "engine/MoveRequest.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/ConnectionManager.hpp"
#include "server/application/EloUpdateUseCase.hpp"
#include "server/application/GameSession.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/domain_ports/IUserRepository.hpp"

namespace {

// Local fake, matching this codebase's per-test-file fake convention (see
// LoginUseCase.tests.cpp for the same shape). Tracks updateElo call count so
// idempotency (no repeat writes across ticks) is directly observable.
class FakeUserRepository : public IUserRepository {
public:
    std::optional<UserRecord> findByUsername(const std::string& username) override {
        const auto it = users_.find(username);
        if (it == users_.end()) return std::nullopt;
        return it->second;
    }
    void createUser(const std::string& username, const std::string& passwordHash) override {
        users_[username] = UserRecord{username, passwordHash, 1200};
    }
    void updateElo(const std::string& username, int newElo) override {
        ++updateEloCalls;
        const auto it = users_.find(username);
        if (it != users_.end()) it->second.elo = newElo;
    }

    int updateEloCalls = 0;

private:
    std::unordered_map<std::string, UserRecord> users_;
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

TEST_CASE("EloUpdateUseCase: a king-capture ending updates both ratings exactly once") {
    GameSession session(GameEngine(makeBoard({{"wR", ".", "bK"}}), registry));
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);
    sessions.registerNew("alice", 'w', &session, "white-conn");
    sessions.registerNew("bob", 'b', &session, "black-conn");

    FakeUserRepository users;
    users.createUser("alice", "hash-a");
    users.createUser("bob", "hash-b");

    EloUpdateUseCase useCase(users, sessions, connections);

    REQUIRE(session.requestMove(MoveRequest{Position{0, 0}, Position{0, 2}}).accepted);
    session.wait(2000);
    REQUIRE(session.snapshot().gameOver);

    useCase.checkAndApply(session);

    CHECK(users.updateEloCalls == 2);
    REQUIRE(users.findByUsername("alice").has_value());
    REQUIRE(users.findByUsername("bob").has_value());
    CHECK(users.findByUsername("alice")->elo == 1216);  // winner (white)
    CHECK(users.findByUsername("bob")->elo == 1184);    // loser (black)

    REQUIRE(connections.sessionFor("white-conn").has_value());
    REQUIRE(connections.sessionFor("black-conn").has_value());
    CHECK(connections.sessionFor("white-conn")->elo == 1216);
    CHECK(connections.sessionFor("black-conn")->elo == 1184);

    // A repeated call (next tick) must not re-apply - gameOver stays true
    // forever after, so without the idempotency guard this would recompute
    // and re-persist Elo every tick indefinitely.
    useCase.checkAndApply(session);
    useCase.checkAndApply(session);

    CHECK(users.updateEloCalls == 2);
    CHECK(users.findByUsername("alice")->elo == 1216);
    CHECK(users.findByUsername("bob")->elo == 1184);
}

TEST_CASE("EloUpdateUseCase: a resignation also triggers an Elo update") {
    GameSession session(GameEngine(makeBoard({{"wR", ".", ".", "."}}), registry));
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);
    sessions.registerNew("alice", 'w', &session, "white-conn");
    sessions.registerNew("bob", 'b', &session, "black-conn");

    FakeUserRepository users;
    users.createUser("alice", "hash-a");
    users.createUser("bob", "hash-b");

    EloUpdateUseCase useCase(users, sessions, connections);

    REQUIRE(session.resign('w'));  // white resigns - black (bob) wins

    useCase.checkAndApply(session);

    CHECK(users.updateEloCalls == 2);
    REQUIRE(users.findByUsername("bob").has_value());
    REQUIRE(users.findByUsername("alice").has_value());
    CHECK(users.findByUsername("bob")->elo == 1216);    // winner
    CHECK(users.findByUsername("alice")->elo == 1184);  // loser
}

TEST_CASE("EloUpdateUseCase: a rating-ineligible session never touches Elo, even after it ends") {
    GameSession session(GameEngine(makeBoard({{"wR", ".", ".", "."}}), registry), /*ratingEligible=*/false);
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    connections.onConnected("white-conn", &session, sessions);
    connections.onConnected("black-conn", &session, sessions);
    sessions.registerNew("alice", 'w', &session, "white-conn");
    sessions.registerNew("bob", 'b', &session, "black-conn");

    FakeUserRepository users;
    users.createUser("alice", "hash-a");
    users.createUser("bob", "hash-b");

    EloUpdateUseCase useCase(users, sessions, connections);

    REQUIRE(session.resign('w'));
    useCase.checkAndApply(session);

    CHECK(users.updateEloCalls == 0);
    CHECK(users.findByUsername("alice")->elo == 1200);
    CHECK(users.findByUsername("bob")->elo == 1200);
}

TEST_CASE("EloUpdateUseCase: checkAndApply on a game still in progress is a no-op") {
    GameSession session(GameEngine(makeBoard({{"wR", ".", ".", "."}}), registry));
    ConnectionManager connections;
    FakeTokenGenerator tokens;
    PlayerSessionRegistry sessions(tokens);
    FakeUserRepository users;

    EloUpdateUseCase useCase(users, sessions, connections);

    useCase.checkAndApply(session);

    CHECK(users.updateEloCalls == 0);
}
