#pragma once

#include <string>
#include <unordered_map>

struct AnimationConfig
{
    double speedMPerSec = 0.0;
    std::string nextStateWhenFinished = "idle";
    int framesPerSec = 6;
    bool isLoop = true;
};

// Returns defaults (see above) if the file is missing or empty, rather
// than throwing - a piece/state with no config.json yet should degrade
// to "doesn't animate", not crash the whole render loop.
AnimationConfig loadAnimationConfig(const std::string &path);

class AnimationConfigLoader
{
public:
    
    explicit AnimationConfigLoader(std::string basePath);
 
    
    const AnimationConfig &configFor(const std::string &pieceCode, const std::string &state);
 
private:
    std::string basePath_;
    std::unordered_map<std::string, AnimationConfig> cache_;
};