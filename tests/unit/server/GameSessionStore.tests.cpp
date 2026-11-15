#include "doctest.h"

#include <vector>

#include "engine/GameEngine.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/GameSessionStore.hpp"

namespace {

pieceRules::PieceRulesRegistry registry;

Board makeBoard() {
    RawBoard raw{{"wK", "."}, {".", "bK"}};
    return buildBoard(raw);
}

}  // namespace

TEST_CASE("GameSessionStore: create() calls the factory exactly once per session") {
    int factoryCalls = 0;
    GameSessionStore store([&]() {
        ++factoryCalls;
        return GameEngine(makeBoard(), registry);
    });

    store.create(/*ratingEligible=*/true);
    CHECK(factoryCalls == 1);

    store.create(/*ratingEligible=*/false);
    CHECK(factoryCalls == 2);
}

TEST_CASE("GameSessionStore: allSessions returns every session created, in creation order") {
    GameSessionStore store([]() { return GameEngine(makeBoard(), registry); });

    GameSession& first = store.create(true);
    GameSession& second = store.create(false);
    GameSession& third = store.create(true);

    const auto all = store.allSessions();
    REQUIRE(all.size() == 3);
    CHECK(all[0] == &first);
    CHECK(all[1] == &second);
    CHECK(all[2] == &third);
}

TEST_CASE("GameSessionStore: a session's address stays stable across many later create() calls") {
    // ConnectionManager/PlayerSessionRegistry hold raw GameSession* into
    // sessions owned by this store - if the vector reallocated the
    // GameSession objects themselves (not just the unique_ptrs), those
    // pointers would dangle. Force a reallocation by creating enough
    // sessions to exceed std::vector's typical initial capacity.
    GameSessionStore store([]() { return GameEngine(makeBoard(), registry); });

    GameSession& firstSession = store.create(true);
    GameSession* firstAddress = &firstSession;

    for (int i = 0; i < 200; ++i) {
        store.create(true);
    }

    CHECK(&firstSession == firstAddress);
    CHECK(store.allSessions().front() == firstAddress);
}
