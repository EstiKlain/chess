#pragma once

#include <functional>
#include <string>
#include <vector>

#include "engine/GameSnapshot.hpp"

struct AnimatedPlacement
{
    std::string pieceCode; 
    std::string state;    
    int frameIndex;        
    int pixelX;
    int pixelY;
};

struct AnimationSpec
{
    int framesPerSec;
    int frameCount;
    bool isLoop = true;
};

using AnimationLookup = std::function<AnimationSpec(const std::string &pieceCode, const std::string &state)>;

namespace PieceAnimator
{
    AnimatedPlacement computePlacement(const PieceSnapshot &piece, long nowMs, int cellSize, const AnimationSpec &moveSpec);

    std::vector<AnimatedPlacement> computePlacements(const GameSnapshot &snapshot, int cellSize, const AnimationLookup &lookup);
}