#pragma once

#include <optional>
#include <vector>

#include "realtime/Moves.hpp"
#include "rules/PieceRules.hpp"
#include "model/Board.hpp"

// Multiple motions can be active concurrently, one per piece.
// Per-piece guards (isPieceInFlight/hasActiveJumpAt/isPieceResting)
// are what stop a single piece from getting a second command mid-flight;
// nothing here limits how many *different* pieces move at once
class RealTimeArbiter
{
public:
    // Advances resolution: lands/captures/promotes any move whose time is
    // up, and drops any jump that has landed. Mutates the board in place.
    // Returns snapshots of the pieces captured this call (used by
    // GameEngine to check for a king capture / game over).
    std::vector<Piece> resolveMoves(Board &board, long elapsedMs, const pieceRules::PieceRulesRegistry &registry);

    bool isPieceInFlight(int row, int col) const;

    // Registers a validated move as in-flight and marks the piece as
    // Moving. Called by GameEngine only after RuleEngine has already
    // accepted the move.
    void startMotion(Board &board, const PieceMove &move);

    bool hasActiveJumpAt(int row, int col) const;
    void startJump(Board &board, const JumpMove &jump);

    std::optional<PieceMove> activeMoveForPiece(int pieceId) const;
    std::optional<JumpMove> activeJumpForPiece(int pieceId) const;

    bool isPieceResting(int pieceId) const;
    std::optional<RestWindow> activeRestForPiece(int pieceId) const;

private:
    std::vector<PieceMove> activeMoves_;
    std::vector<JumpMove> activeJumps_;
    std::vector<RestWindow> activeRests_;

    void startRest(Board &board, int pieceId, long startMs, long durationMs, PieceState kind); // NEW

    // Detects and resolves collisions between two concurrently in-flight
    // moves *before* either one has actually landed - not just at their
    // final destinations. Only compared for moves whose PieceMove::path is
    // non-empty - pre-computed once by GameEngine from the mover's own
    // MoveRule::pathCells at request time (empty for a knight's hop or any
    // other move with no intermediate cell, which keeps today's
    // landing-only collision behavior unchanged). This function reads only
    // plain data already sitting on the two PieceMoves - it never consults
    // PieceRulesRegistry, and doesn't even know what a "piece kind" is:
    // real-time collision timing is a geometric/timing fact about an
    // already-approved move, not a game-legality decision, so it stays
    // fully independent of the rules table RuleEngine/Movement use to
    // decide what's allowed.
    //
    // Different colors sharing a cell at (near-)the same instant: the
    // later arrival captures the earlier one. Same color: the later mover
    // is stopped one cell short of the shared cell, along its own path
    // (truncateMotion) - never the other mover's path. On an EXACT tie
    // (arrival times equal - not rare, since GameEngine quantizes startMs
    // to the caller's own tick) neither is "later": different colors ->
    // mutual capture, same color -> both stop short. See
    // rules/CollisionRule.hpp's decideCollisionOutcome for the full rule.
    //
    // A conflict is only acted on once elapsedMs has reached the earlier
    // of the two movers' scheduled arrivals at the shared cell - mirrors
    // resolveMoves' own "due" gating for ordinary arrivals, so a collision
    // scheduled for the future doesn't resolve the instant both moves are
    // merely registered.
    //
    // Returns snapshots of any pieces captured this way, so the caller
    // (resolveMoves) can fold them into the same captured-list it already
    // returns for arrival-time captures (GameEngine checks that list for a
    // king capture / game over).
    std::vector<Piece> resolveMidFlightCollisions(Board &board, long elapsedMs);

    // Rewrites an in-flight move to end at newPath.back() instead of its
    // original toRow/toCol, replaces move.path with newPath (a strict
    // prefix of the move's own original path), and shortens durationMs to
    // match - keeping the mover's original speed. Never called with an
    // empty newPath - that "stuck before taking even one step" case is
    // cancelMotion's job instead.
    void truncateMotion(PieceMove &move, std::vector<Position> newPath, long newDurationMs);

    // Cancels an in-flight move outright - the mover never even reaches
    // its first path cell before a same-color collision. Mirrors the
    // existing "friendly piece blocks destination" fallback in
    // resolveMoves (stays Idle at its current cell), just reachable
    // before arrival instead of at it.
    void cancelMotion(Board &board, int pieceId);
};
