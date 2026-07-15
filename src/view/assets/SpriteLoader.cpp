#include "SpriteLoader.hpp"

SpriteLoader::SpriteLoader(std::string basePath)
    : basePath_(std::move(basePath))
{
}

const Img &SpriteLoader::idleSprite(const std::string &pieceCode, int cellSize)
{
    auto it = cache_.find(pieceCode);
    if (it != cache_.end())
        return it->second;

    const std::string path = basePath_ + "/" + pieceCode + "/states/idle/sprites/1.png";

    Img sprite;
    sprite.read(path, {cellSize, cellSize}); // square sprites, exact fit, no keep_aspect needed

    auto result = cache_.emplace(pieceCode, std::move(sprite));
    return result.first->second;
}