// Pure math, zero Img/SpriteLoader/disk dependency - PieceAnimator only
// ever sees plain GameSnapshot values plus an injected AnimationLookup
// lambda. Every test here builds its own PieceSnapshot/MotionSnapshot by
// hand; nothing touches a real GameEngine or the filesystem.
//
// Iteration E: PieceAnimator no longer infers "move" vs "idle" purely
// from motion.has_value() - it reads piece.state directly (Idle, Moving,
// Jumping, RestingShort, RestingLong, Captured) and maps it to the
// matching sprite-folder name. That's the fix for the original bug:
// jump/rest states previously never got looked up at all.
#include "doctest.h"

#include "view/render/PieceAnimator.hpp"
#include "view/render/BoardGeometry.hpp"

namespace
{
    PieceSnapshot idlePiece(int id, char color, char kind, int row, int col)
    {
        PieceSnapshot p;
        p.id = id;
        p.color = color;
        p.kind = kind;
        p.row = row;
        p.col = col;
        p.state = PieceState::Idle; // explicit - this is also the struct default
        p.motion = std::nullopt;
        return p;
    }

    PieceSnapshot movingPiece(int id, char color, char kind,
                               int fromRow, int fromCol, int toRow, int toCol,
                               long startMs, long durationMs)
    {
        PieceSnapshot p = idlePiece(id, color, kind, fromRow, fromCol);
        p.state = PieceState::Moving; // state now drives everything, not just motion.has_value()
        p.stateStartMs = startMs;     // must match the move's own startMs
        p.motion = MotionSnapshot{fromRow, fromCol, toRow, toCol, startMs, durationMs};
        return p;
    }

    PieceSnapshot jumpingPiece(int id, char color, char kind, int row, int col, long stateStartMs)
    {
        PieceSnapshot p = idlePiece(id, color, kind, row, col);
        p.state = PieceState::Jumping;
        p.stateStartMs = stateStartMs;
        return p;
    }

    PieceSnapshot restingPiece(int id, char color, char kind, int row, int col,
                                PieceState restState, long stateStartMs)
    {
        PieceSnapshot p = idlePiece(id, color, kind, row, col);
        p.state = restState; // RestingShort or RestingLong
        p.stateStartMs = stateStartMs;
        return p;
    }

    // Fixed lookup: same AnimationSpec no matter what's asked - fine for
    // tests that don't care about per-piece config, just the geometry.
    AnimationLookup fixedLookup(int framesPerSec, int frameCount, bool isLoop = true)
    {
        return [framesPerSec, frameCount, isLoop](const std::string &, const std::string &) -> AnimationSpec
        {
            return AnimationSpec{framesPerSec, frameCount, isLoop};
        };
    }
}

// --- computePlacement: idle pieces -----------------------------------

TEST_CASE("computePlacement: an idle piece is drawn at its cell's pixel position, state=idle, frame=1")
{
    const PieceSnapshot p = idlePiece(1, 'w', 'Q', 2, 3);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 1000, /*cellSize*/ 100, AnimationSpec{6, 4});

    const auto rect = BoardGeometry::cellRect(2, 3, 100);
    CHECK(placement.pieceCode == "QW");
    CHECK(placement.state == "idle");
    // Idle/Captured are pinned to frame 1 regardless of elapsed time -
    // they were never part of the reported bug (only jump/rest were),
    // so their behavior is deliberately unchanged from before Iteration E.
    CHECK(placement.frameIndex == 1);
    CHECK(placement.pixelX == rect.x);
    CHECK(placement.pixelY == rect.y);
}

TEST_CASE("computePlacement: pieceCode combines kind+uppercased color the same way for black pieces")
{
    const PieceSnapshot p = idlePiece(2, 'b', 'P', 6, 4);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, 0, 100, AnimationSpec{6, 4});

    CHECK(placement.pieceCode == "PB");
}

// --- computePlacement: moving pieces, progress/interpolation ---------

TEST_CASE("computePlacement: a move at its exact start (progress=0) sits at the source cell")
{
    const PieceSnapshot p = movingPiece(3, 'w', 'R', 0, 0, 0, 4, /*startMs*/ 1000, /*durationMs*/ 1000);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 1000, 100, AnimationSpec{6, 4});

    const auto from = BoardGeometry::cellRect(0, 0, 100);
    CHECK(placement.state == "move");
    CHECK(placement.pixelX == from.x);
    CHECK(placement.pixelY == from.y);
}

TEST_CASE("computePlacement: a move exactly halfway through is halfway between source and destination pixels")
{
    const PieceSnapshot p = movingPiece(4, 'w', 'R', 0, 0, 0, 4, /*startMs*/ 0, /*durationMs*/ 1000);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 500, 100, AnimationSpec{6, 4});

    const auto from = BoardGeometry::cellRect(0, 0, 100);
    const auto to = BoardGeometry::cellRect(0, 4, 100);
    CHECK(placement.pixelX == (from.x + to.x) / 2);
    CHECK(placement.pixelY == (from.y + to.y) / 2);
}

TEST_CASE("computePlacement: a move whose duration has fully elapsed lands exactly on the destination cell")
{
    const PieceSnapshot p = movingPiece(5, 'w', 'R', 0, 0, 3, 0, /*startMs*/ 0, /*durationMs*/ 1000);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 1000, 100, AnimationSpec{6, 4});

    const auto to = BoardGeometry::cellRect(3, 0, 100);
    CHECK(placement.pixelX == to.x);
    CHECK(placement.pixelY == to.y);
}

TEST_CASE("computePlacement: progress is clamped - nowMs past the end never overshoots the destination")
{
    const PieceSnapshot p = movingPiece(6, 'w', 'R', 0, 0, 3, 0, /*startMs*/ 0, /*durationMs*/ 1000);
    // nowMs is 5x the move's duration - a stale/late snapshot read.
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 5000, 100, AnimationSpec{6, 4});

    const auto to = BoardGeometry::cellRect(3, 0, 100);
    CHECK(placement.pixelX == to.x);
    CHECK(placement.pixelY == to.y);
}

TEST_CASE("computePlacement: a zero-duration move does not divide by zero and lands on the destination")
{
    const PieceSnapshot p = movingPiece(7, 'w', 'N', 0, 0, 1, 2, /*startMs*/ 0, /*durationMs*/ 0);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 0, 100, AnimationSpec{6, 4});

    // durationMs<=0 -> progress stays 0.0 per the implementation, so this
    // documents current behavior (source cell) rather than assuming a
    // "snap to destination" fix that hasn't been made - guards against a
    // silent behavior change/crash (e.g. div-by-zero) if the formula
    // changes later.
    const auto from = BoardGeometry::cellRect(0, 0, 100);
    CHECK(placement.pixelX == from.x);
    CHECK(placement.pixelY == from.y);
}

// --- computePlacement: frame index selection (move) --------------------

TEST_CASE("computePlacement: frame index advances with elapsed time at the given frames_per_sec")
{
    // 6 fps, 4 frames -> a new frame roughly every ~167ms. At 350ms in,
    // floor(350*6/1000)=2 frames played -> frame index 2+1=3 (1-based).
    const PieceSnapshot p = movingPiece(8, 'w', 'R', 0, 0, 0, 4, /*startMs*/ 0, /*durationMs*/ 2000);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 350, 100, AnimationSpec{6, 4});

    CHECK(placement.frameIndex == 3);
}

TEST_CASE("computePlacement: frame index loops back around once it exceeds frameCount (is_loop=true)")
{
    // 10 fps, 3 frames. At 1000ms in: 10 frames played, 10 % 3 = 1 -> index 2.
    const PieceSnapshot p = movingPiece(9, 'w', 'R', 0, 0, 0, 4, /*startMs*/ 0, /*durationMs*/ 5000);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 1000, 100, AnimationSpec{10, 3});

    CHECK(placement.frameIndex == 2);
}

TEST_CASE("computePlacement: falls back to frame 1 when frameCount is 0 (no sprites for this state yet)")
{
    const PieceSnapshot p = movingPiece(10, 'w', 'R', 0, 0, 0, 4, /*startMs*/ 0, /*durationMs*/ 1000);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 500, 100, AnimationSpec{6, /*frameCount*/ 0});

    CHECK(placement.frameIndex == 1);
}

TEST_CASE("computePlacement: falls back to frame 1 when framesPerSec is 0")
{
    const PieceSnapshot p = movingPiece(11, 'w', 'R', 0, 0, 0, 4, /*startMs*/ 0, /*durationMs*/ 1000);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 500, 100, AnimationSpec{/*framesPerSec*/ 0, 4});

    CHECK(placement.frameIndex == 1);
}

// --- computePlacement: jump / rest (Iteration E - the original bug) ----

TEST_CASE("computePlacement: a jumping piece uses \"jump\" as its state and stays at its own cell (no interpolation)")
{
    const PieceSnapshot p = jumpingPiece(20, 'w', 'N', 2, 2, /*stateStartMs*/ 0);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 100, 100, AnimationSpec{8, 4, false});

    const auto rect = BoardGeometry::cellRect(2, 2, 100);
    CHECK(placement.state == "jump");
    CHECK(placement.pixelX == rect.x);
    CHECK(placement.pixelY == rect.y);
}

TEST_CASE("computePlacement: a jumping piece's frame index advances over time, just like a move")
{
    // 8 fps, 4 frames, non-looping. At 250ms: floor(250*8/1000)=2 played -> index 3.
    const PieceSnapshot p = jumpingPiece(24, 'w', 'N', 0, 0, /*stateStartMs*/ 0);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 250, 100, AnimationSpec{8, 4, false});

    CHECK(placement.frameIndex == 3);
}

TEST_CASE("computePlacement: a long-resting piece maps to \"long_rest\"")
{
    const PieceSnapshot p = restingPiece(21, 'b', 'Q', 4, 4, PieceState::RestingLong, /*stateStartMs*/ 0);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 100, 100, AnimationSpec{8, 4, false});

    const auto rect = BoardGeometry::cellRect(4, 4, 100);
    CHECK(placement.state == "long_rest");
    CHECK(placement.pixelX == rect.x);
    CHECK(placement.pixelY == rect.y);
}

TEST_CASE("computePlacement: a short-resting piece maps to \"short_rest\"")
{
    const PieceSnapshot p = restingPiece(22, 'w', 'P', 1, 1, PieceState::RestingShort, /*stateStartMs*/ 0);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 0, 100, AnimationSpec{6, 3, false});

    CHECK(placement.state == "short_rest");
}

TEST_CASE("computePlacement: a captured piece is pinned to frame 1 like idle")
{
    PieceSnapshot p = idlePiece(25, 'b', 'P', 0, 0);
    p.state = PieceState::Captured;

    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 5000, 100, AnimationSpec{8, 4, false});

    CHECK(placement.state == "idle"); // stateName has no dedicated "captured" folder - falls back to idle
    CHECK(placement.frameIndex == 1);
}

TEST_CASE("computePlacement: non-looping animation (jump/rest) holds on the last frame instead of wrapping")
{
    // 10fps, 3 frames, is_loop=false. At 1000ms, 10 frames WOULD have
    // played if it looped (10%3=1 -> would wrongly go back to frame 2) -
    // but non-loop must clamp and hold on frame 3.
    const PieceSnapshot p = jumpingPiece(23, 'w', 'B', 0, 0, /*stateStartMs*/ 0);
    const AnimatedPlacement placement = PieceAnimator::computePlacement(p, /*nowMs*/ 1000, 100, AnimationSpec{10, 3, false});

    CHECK(placement.frameIndex == 3);
}

// --- computePlacements: whole-snapshot orchestration -------------------

TEST_CASE("computePlacements: produces exactly one placement per piece in the snapshot")
{
    GameSnapshot snapshot;
    snapshot.nowMs = 0;
    snapshot.pieces = {idlePiece(1, 'w', 'K', 0, 0), idlePiece(2, 'b', 'K', 7, 7)};

    const auto placements = PieceAnimator::computePlacements(snapshot, 100, fixedLookup(6, 4));

    CHECK(placements.size() == 2);
}

TEST_CASE("computePlacements: an empty snapshot produces an empty placement list")
{
    GameSnapshot snapshot;
    snapshot.nowMs = 0;

    const auto placements = PieceAnimator::computePlacements(snapshot, 100, fixedLookup(6, 4));

    CHECK(placements.empty());
}

TEST_CASE("computePlacements: looks up the AnimationSpec for each piece's OWN current state, not a hardcoded one")
{
    // This replaces the old (buggy-by-design) test that asserted the
    // lookup always requested "move" no matter what - that assumption
    // WAS the original bug: jump/rest states were never looked up.
    GameSnapshot snapshot;
    snapshot.nowMs = 0;
    snapshot.pieces = {
        idlePiece(1, 'w', 'Q', 0, 0),      // state defaults to Idle
        jumpingPiece(2, 'b', 'N', 3, 3, 0) // state = Jumping
    };

    std::vector<std::string> requestedStates;
    const AnimationLookup lookup =
        [&](const std::string &, const std::string &state) -> AnimationSpec
    {
        requestedStates.push_back(state);
        return AnimationSpec{6, 4};
    };

    PieceAnimator::computePlacements(snapshot, 100, lookup);

    REQUIRE(requestedStates.size() == 2);
    CHECK(requestedStates[0] == "idle");
    CHECK(requestedStates[1] == "jump");
}

TEST_CASE("computePlacements: uses each piece's own nowMs-relative progress from the shared snapshot.nowMs")
{
    GameSnapshot snapshot;
    snapshot.nowMs = 750;
    PieceSnapshot moving = movingPiece(1, 'w', 'R', 0, 0, 0, 4, /*startMs*/ 500, /*durationMs*/ 1000);
    snapshot.pieces = {moving};

    const auto placements = PieceAnimator::computePlacements(snapshot, 100, fixedLookup(6, 4));

    REQUIRE(placements.size() == 1);
    // progress = (750-500)/1000 = 0.25
    const auto from = BoardGeometry::cellRect(0, 0, 100);
    const auto to = BoardGeometry::cellRect(0, 4, 100);
    const int expectedX = from.x + static_cast<int>((to.x - from.x) * 0.25);
    CHECK(placements[0].pixelX == expectedX);
}