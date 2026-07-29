#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "engine/GameEngine.hpp"
#include "server/application/GameSession.hpp"

// Owns every GameSession for the process's lifetime.
//
// unique_ptr in the vector, not GameSession by value: ConnectionManager
// and PlayerSessionRegistry hold raw GameSession* into sessions created
// here, and reallocating a vector<GameSession> would move the objects
// themselves. unique_ptr keeps the pointee's address stable; only the
// pointer moves.
//
// No removal is implemented, deliberately: those raw pointers make
// deleting a GameSession a use-after-free risk, not just an address
// change. Adding removal needs a real ownership model first (see
// CLAUDE.md's Server-Iteration 7 notes).
//
// No mutex: this runs on the single io_context thread (CLAUDE.md,
// Iteration 7 threading decision) - no second thread ever touches this.
class GameSessionStore {
public:
    /// engineFactory builds a fresh GameEngine (board + rules) for each new session - kept as a factory rather than loading assets.opening_board.txt inline here so file IO happens once, wired by the composition root, and tests can supply a trivial in-memory factory instead.
    explicit GameSessionStore(std::function<GameEngine()> engineFactory);

    /// Creates a new session; the returned reference's address is stable for the process's remaining lifetime (see the class comment above).
    GameSession& create(bool ratingEligible);

    /// Every session created so far, in creation order.
    std::vector<GameSession*> allSessions() const;

private:
    std::function<GameEngine()> engineFactory_;
    std::vector<std::unique_ptr<GameSession>> sessions_;
};
