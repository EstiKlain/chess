#include "SpriteLoader.hpp"

#include <filesystem>

SpriteLoader::SpriteLoader(std::string basePath)
    : basePath_(std::move(basePath))
{
}

const Img &SpriteLoader::idleSprite(const std::string &pieceCode, int cellSize)
{
    return frame(pieceCode, "idle", 1, cellSize);
}

const Img &SpriteLoader::frame(const std::string &pieceCode, const std::string &state, int frameIndex, int cellSize)
{
    const std::string key = pieceCode + "/" + state + "/" + std::to_string(frameIndex) + "/" + std::to_string(cellSize);

    auto it = cache_.find(key);
    if (it != cache_.end())
        return it->second;

    const std::string path = basePath_ + "/" + pieceCode + "/states/" + state +
                              "/sprites/" + std::to_string(frameIndex) + ".png";

    Img sprite;
    sprite.read(path, {cellSize, cellSize}); // square sprites, exact fit, no keep_aspect needed

    auto result = cache_.emplace(key, std::move(sprite));
    return result.first->second;
}

int SpriteLoader::frameCount(const std::string &pieceCode, const std::string &state)
{
    const std::string key = pieceCode + "/" + state;

    auto it = frameCountCache_.find(key);
    if (it != frameCountCache_.end())
        return it->second;

    const std::string dir = basePath_ + "/" + pieceCode + "/states/" + state + "/sprites";

    int count = 0;
    if (std::filesystem::exists(dir))
    {
        for (const auto &entry : std::filesystem::directory_iterator(dir))
            if (entry.path().extension() == ".png")
                ++count;
    }

    frameCountCache_[key] = count;
    return count;
}