#pragma once

#include <string>
#include <unordered_map>

#include "view/canvas/img.hpp"

class SpriteLoader
{
public:
    explicit SpriteLoader(std::string basePath);

    const Img &idleSprite(const std::string &pieceCode, int cellSize);

    const Img &frame(const std::string &pieceCode, const std::string &state, int frameIndex, int cellSize);

    int frameCount(const std::string &pieceCode, const std::string &state);

private:
    std::string basePath_;
    std::unordered_map<std::string, Img> cache_;
    std::unordered_map<std::string, int> frameCountCache_;
};