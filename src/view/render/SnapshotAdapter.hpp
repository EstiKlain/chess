#pragma once

#include <vector>

#include "PiecePlacement.hpp"
#include "engine/GameSnapshot.hpp"

// Pure translation layer, Iteration D: converts the engine's
// (kind, color) representation into the view's pieceCode string
// convention that SpriteLoader/BoardRenderer already expect from
// Iteration B - e.g. kind='Q', color='w' -> "QW", matching the
// assets/pieces_classic/QW/... folder name exactly.
//
// This exists so BoardRenderer::drawPieces (already written and tested
// in Iteration B) needs ZERO changes for Iteration D: it still only
// ever sees vector<PiecePlacement>, it has no idea a GameEngine exists.
namespace SnapshotAdapter
{
    std::vector<PiecePlacement> toPlacements(const GameSnapshot &snapshot);
}
