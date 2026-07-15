#pragma once

#include <string>
#include <unordered_map>

#include "view/canvas/img.hpp"

class SpriteLoader
{
public:
    
    explicit SpriteLoader(std::string basePath);

    const Img &idleSprite(const std::string &pieceCode, int cellSize);

private:
    std::string basePath_;
    std::unordered_map<std::string, Img> cache_;
};