#pragma once

#include <vector>

#include "PiecePlacement.hpp"
#include "engine/GameSnapshot.hpp"

// Pure translation layer
namespace SnapshotAdapter
{
    std::vector<PiecePlacement> toPlacements(const GameSnapshot &snapshot);
}
