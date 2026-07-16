#include "AnimationConfig.hpp"

#include <fstream>
#include <sstream>

namespace
{
    // Finds "key": value and returns the raw value text, trimmed of
    // whitespace/quotes, stopping at the next comma/brace/newline. Good
    // enough for this flat, fixed-shape config.json - not a general
    // JSON parser.
    std::string findValue(const std::string &text, const std::string &key)
    {
        const std::string needle = "\"" + key + "\"";
        size_t pos = text.find(needle);
        if (pos == std::string::npos)
            return "";

        pos = text.find(':', pos);
        if (pos == std::string::npos)
            return "";
        ++pos;

        const size_t end = text.find_first_of(",}\n", pos);
        std::string value = text.substr(pos, end - pos);

        const size_t a = value.find_first_not_of(" \t\"");
        const size_t b = value.find_last_not_of(" \t\"");
        if (a == std::string::npos)
            return "";
        return value.substr(a, b - a + 1);
    }
}

AnimationConfig loadAnimationConfig(const std::string &path)
{
    AnimationConfig config; 

    std::ifstream file(path);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string text = buffer.str();

    if (text.empty())
        return config;

    if (const std::string v = findValue(text, "speed_m_per_sec"); !v.empty())
        config.speedMPerSec = std::stod(v);

    if (const std::string v = findValue(text, "next_state_when_finished"); !v.empty())
        config.nextStateWhenFinished = v;

    if (const std::string v = findValue(text, "frames_per_sec"); !v.empty())
        config.framesPerSec = std::stoi(v);

    if (const std::string v = findValue(text, "is_loop"); !v.empty())
        config.isLoop = (v == "true");

    return config;
}

AnimationConfigLoader::AnimationConfigLoader(std::string basePath)
    : basePath_(std::move(basePath))
{
}
 
const AnimationConfig &AnimationConfigLoader::configFor(const std::string &pieceCode, const std::string &state)
{
    const std::string key = pieceCode + "/" + state;
 
    auto it = cache_.find(key);
    if (it != cache_.end())
        return it->second;
 
    const std::string path = basePath_ + "/" + pieceCode + "/states/" + state + "/config.json";
 
    auto result = cache_.emplace(key, loadAnimationConfig(path));
    return result.first->second;
}