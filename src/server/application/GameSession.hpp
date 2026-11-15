#pragma once

#include <mutex>

#include "engine/GameEngine.hpp"
#include "engine/GameSnapshot.hpp"
#include "engine/MoveRequest.hpp"

// Wraps exactly one GameEngine for one game. The mutex here is a
// network-level concurrency guard, not a game-rule one: piece cooldowns are
// already handled inside GameEngine/RealTimeArbiter and are unaffected by
// this lock. What this lock prevents is two network connections calling into
// the same GameEngine instance from different threads at the same instant
// (a memory race, not a rules race) - see the plan's decisions log entry on
// network-level vs. game-rule concurrency.
class GameSession {
public:
    /// @param engine The GameEngine this session wraps.
    /// @param ratingEligible Whether a decisive ending of this game should update Elo (see EloUpdateUseCase). Defaults to true; Room-origin games (Iteration 8) will construct with false. A plain bool rather than a GameOrigin{Play,Room} enum on purpose - EloUpdateUseCase only ever needs "does this count," never "what kind of game is this," so future game types (tournament, practice, ...) only need to pass the right value here, with zero changes to EloUpdateUseCase itself.
    explicit GameSession(GameEngine engine, bool ratingEligible = true)
        : engine_(std::move(engine)), ratingEligible_(ratingEligible) {}

    /// Whether a decisive ending of this game should trigger an Elo update.
    bool ratingEligible() const { return ratingEligible_; }

    /// Forwards to GameEngine::requestMove under the session's lock.
    MoveResult requestMove(const MoveRequest& request);

    /// Forwards to GameEngine::requestJump under the session's lock. color is the caller's asserted piece color (see MoveRequest::requesterColor for the '\0' sentinel convention).
    JumpResult requestJump(int row, int col, char color = '\0');

    /// Forwards to GameEngine::snapshot under the session's lock.
    GameSnapshot snapshot() const;

    /// Forwards to GameEngine::wait under the session's lock - advances the game clock so in-flight motions/cooldowns resolve. Called periodically by main_server.cpp's tick loop, never by a request handler.
    void wait(long ms);

    /// Forwards to GameEngine::resign under the session's lock - required, not optional: MakeMoveUseCase's requestMove/requestJump and the tick thread's wait() already only ever touch GameEngine through this lock, so DisconnectUseCase::tick must too, or it would reintroduce exactly the cross-thread race this lock exists to prevent.
    bool resign(char color);

private:
    mutable std::mutex mutex_;
    GameEngine engine_;
    bool ratingEligible_;
};
