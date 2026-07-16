#include "PieceAnimator.hpp"

#include <algorithm>
#include <cctype>

#include "BoardGeometry.hpp"

namespace
{
    std::string pieceCodeOf(const PieceSnapshot &p)
    {
        return std::string(1, p.kind) +
               std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(p.color))));
    }

    // NEW: maps the real PieceState to the folder-name convention under
    // assets/pieces_classic/<code>/states/<name>/ - this is the piece that was
    // completely missing before, so jump/rest never got looked up.
    std::string stateName(PieceState s)
    {
        switch (s)
        {
        case PieceState::Moving:
            return "move";
        case PieceState::Jumping:
            return "jump";
        case PieceState::RestingShort:
            return "short_rest";
        case PieceState::RestingLong:
            return "long_rest";
        case PieceState::Idle:
        case PieceState::Captured:
        default:
            return "idle";
        }
    }

    int lerp(int a, int b, double t)
    {
        return static_cast<int>(a + (b - a) * t);
    }
}

namespace PieceAnimator
{
    AnimatedPlacement computePlacement(const PieceSnapshot &piece, long nowMs, int cellSize, const AnimationSpec &stateSpec)
    {
        AnimatedPlacement result;
        result.pieceCode = pieceCodeOf(piece);
        result.state = stateName(piece.state);

        if (piece.motion.has_value())
        {
            const MotionSnapshot &m = *piece.motion;
            double progress = 0.0;
            if (m.durationMs > 0)
                progress = static_cast<double>(nowMs - m.startMs) / static_cast<double>(m.durationMs);
            progress = std::clamp(progress, 0.0, 1.0);

            const auto fromRect = BoardGeometry::cellRect(m.fromRow, m.fromCol, cellSize);
            const auto toRect = BoardGeometry::cellRect(m.toRow, m.toCol, cellSize);
            result.pixelX = lerp(fromRect.x, toRect.x, progress);
            result.pixelY = lerp(fromRect.y, toRect.y, progress);
        }
        else
        {
            // Jump/rest/idle: the piece is visually stationary at its
            // logical cell (jump = "in place", rest = "in place").
            const auto rect = BoardGeometry::cellRect(piece.row, piece.col, cellSize);
            result.pixelX = rect.x;
            result.pixelY = rect.y;
        }

        if (piece.state == PieceState::Idle || piece.state == PieceState::Captured)
        {
            result.frameIndex = 1; 
        }
        else if (stateSpec.frameCount <= 0 || stateSpec.framesPerSec <= 0)
        {
            result.frameIndex = 1;
        }
        else
        {
            const long elapsed = std::max<long>(0, nowMs - piece.stateStartMs);
            const long framesPlayed = (elapsed * stateSpec.framesPerSec) / 1000;

            if (stateSpec.isLoop)
                result.frameIndex = static_cast<int>(framesPlayed % stateSpec.frameCount) + 1;
            else
                // Non-looping (e.g. jump, short_rest, long_rest): play
                // through once, then hold on the last frame instead of
                // snapping back to frame 1.
                result.frameIndex = static_cast<int>(std::min<long>(framesPlayed, stateSpec.frameCount - 1)) + 1;
        }

        return result;
    }

    std::vector<AnimatedPlacement> computePlacements(const GameSnapshot &snapshot, int cellSize, const AnimationLookup &lookup)
    {
        std::vector<AnimatedPlacement> result;
        result.reserve(snapshot.pieces.size());

        for (const PieceSnapshot &p : snapshot.pieces)
        {
            const std::string code = pieceCodeOf(p);
            const std::string state = stateName(p.state); // CHANGED: was always "move"
            const AnimationSpec spec = lookup(code, state);
            result.push_back(computePlacement(p, snapshot.nowMs, cellSize, spec));
        }

        return result;
    }
}