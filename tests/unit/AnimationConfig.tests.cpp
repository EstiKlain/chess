// Pure config-parsing + caching logic, no OpenCV/window involved.
// Same convention as PiecePlacement.tests.cpp: each test writes its own
// tiny scratch config.json (or directory tree of them) so nothing here
// depends on the real assets/pieces_classicclassicclassicclassic folder.
#include "doctest.h"

#include "view/assets/AnimationConfig.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace
{
    // Writes assets/<pieceCode>/states/<state>/config.json under a
    // scratch root, mirroring the real folder shape
    // AnimationConfigLoader::configFor builds paths for. Whole root is
    // removed on destruction - nothing leaks into other tests.
    struct TempConfigTree
    {
        std::filesystem::path root;

        explicit TempConfigTree(const std::string &rootName)
            : root(std::filesystem::temp_directory_path() / rootName)
        {
            std::filesystem::remove_all(root); // start clean if a previous run crashed mid-test
        }

        void write(const std::string &pieceCode, const std::string &state, const std::string &json)
        {
            const std::filesystem::path dir = root / pieceCode / "states" / state;
            std::filesystem::create_directories(dir);
            std::ofstream out(dir / "config.json");
            out << json;
        }

        std::string path(const std::string &pieceCode, const std::string &state) const
        {
            return (root / pieceCode / "states" / state / "config.json").string();
        }

        std::string basePath() const { return root.string(); }

        ~TempConfigTree() { std::filesystem::remove_all(root); }
    };
}

// --- loadAnimationConfig (free function): pure path -> struct parsing ---

TEST_CASE("loadAnimationConfig reads all four fields from a well-formed config.json")
{
    TempConfigTree tree("anim_config_wellformed");
    tree.write("PB", "move", R"({
        "physics": { "speed_m_per_sec": 2.5, "next_state_when_finished": "idle" },
        "graphics": { "frames_per_sec": 6, "is_loop": true }
    })");

    const AnimationConfig config = loadAnimationConfig(tree.path("PB", "move"));

    CHECK(config.speedMPerSec == doctest::Approx(2.5));
    CHECK(config.nextStateWhenFinished == "idle");
    CHECK(config.framesPerSec == 6);
    CHECK(config.isLoop == true);
}

TEST_CASE("loadAnimationConfig reads is_loop=false correctly, not just defaulting to true")
{
    TempConfigTree tree("anim_config_noloop");
    tree.write("QW", "move", R"({"graphics": {"is_loop": false, "frames_per_sec": 10}})");

    const AnimationConfig config = loadAnimationConfig(tree.path("QW", "move"));

    CHECK(config.isLoop == false);
    CHECK(config.framesPerSec == 10);
}

TEST_CASE("loadAnimationConfig returns defaults when the file doesn't exist")
{
    const AnimationConfig config = loadAnimationConfig("/no/such/path/config.json");

    CHECK(config.speedMPerSec == doctest::Approx(0.0));
    CHECK(config.nextStateWhenFinished == "idle");
    CHECK(config.framesPerSec == 6);
    CHECK(config.isLoop == true);
}

TEST_CASE("loadAnimationConfig returns defaults for an empty file instead of throwing")
{
    TempConfigTree tree("anim_config_empty");
    tree.write("KB", "idle", "");

    const AnimationConfig config = loadAnimationConfig(tree.path("KB", "idle"));

    CHECK(config.framesPerSec == 6);
    CHECK(config.nextStateWhenFinished == "idle");
}

TEST_CASE("loadAnimationConfig only fills in fields present, leaving the rest at default")
{
    TempConfigTree tree("anim_config_partial");
    // Only frames_per_sec present - everything else should fall back to
    // AnimationConfig's own default member values.
    tree.write("NB", "move", R"({"graphics": {"frames_per_sec": 12}})");

    const AnimationConfig config = loadAnimationConfig(tree.path("NB", "move"));

    CHECK(config.framesPerSec == 12);
    CHECK(config.speedMPerSec == doctest::Approx(0.0));
    CHECK(config.nextStateWhenFinished == "idle");
    CHECK(config.isLoop == true);
}

// --- AnimationConfigLoader: caching wrapper, same shape as SpriteLoader ---

TEST_CASE("AnimationConfigLoader builds the same path convention as SpriteLoader::frame")
{
    TempConfigTree tree("anim_loader_path");
    tree.write("RW", "move", R"({"graphics": {"frames_per_sec": 8}})");

    AnimationConfigLoader loader(tree.basePath());
    const AnimationConfig &config = loader.configFor("RW", "move");

    CHECK(config.framesPerSec == 8);
}

TEST_CASE("AnimationConfigLoader caches: repeated calls for the same (pieceCode,state) return the same cached object")
{
    TempConfigTree tree("anim_loader_identity");
    tree.write("BB", "move", R"({"graphics": {"frames_per_sec": 4}})");

    AnimationConfigLoader loader(tree.basePath());

    const AnimationConfig &first = loader.configFor("BB", "move");
    const AnimationConfig &second = loader.configFor("BB", "move");

    // Same cached object, not just equal content - proves the second
    // call never touched the loadAnimationConfig/disk path again.
    CHECK(&first == &second);
}

TEST_CASE("AnimationConfigLoader caches: a later on-disk change is NOT picked up (proves no re-read happens)")
{
    TempConfigTree tree("anim_loader_stale");
    tree.write("PW", "move", R"({"graphics": {"frames_per_sec": 5}})");

    AnimationConfigLoader loader(tree.basePath());
    const AnimationConfig first = loader.configFor("PW", "move"); // populates cache, value-copy on purpose

    // Rewrite the file on disk with a different value.
    tree.write("PW", "move", R"({"graphics": {"frames_per_sec": 99}})");
    const AnimationConfig &second = loader.configFor("PW", "move");

    CHECK(first.framesPerSec == 5);
    CHECK(second.framesPerSec == 5); // still the cached value, not 99
}

TEST_CASE("AnimationConfigLoader treats different states for the same piece as separate cache entries")
{
    TempConfigTree tree("anim_loader_states");
    tree.write("KW", "idle", R"({"graphics": {"frames_per_sec": 1}})");
    tree.write("KW", "move", R"({"graphics": {"frames_per_sec": 7}})");

    AnimationConfigLoader loader(tree.basePath());

    CHECK(loader.configFor("KW", "idle").framesPerSec == 1);
    CHECK(loader.configFor("KW", "move").framesPerSec == 7);
}

TEST_CASE("AnimationConfigLoader falls back to defaults for a piece/state with no config.json yet")
{
    TempConfigTree tree("anim_loader_missing");
    // No files written at all.

    AnimationConfigLoader loader(tree.basePath());
    const AnimationConfig &config = loader.configFor("XX", "move");

    CHECK(config.framesPerSec == 6);
    CHECK(config.nextStateWhenFinished == "idle");
}