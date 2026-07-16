// Uses real files under assets/pieces_classic (via PROJECT_ROOT), same convention
// as img_test's manual sprite-loading check -- this just makes it an
// automated doctest instead of a manual run.
#include "doctest.h"

#include "view/assets/SpriteLoader.hpp"

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

TEST_CASE("SpriteLoader returns a non-empty idle sprite for a known piece code")
{
    SpriteLoader loader(std::string(PROJECT_ROOT) + "/assets/pieces_classic");

    const Img &queen = loader.idleSprite("QW", 100);
    CHECK(queen.is_loaded());

    const Img &pawn = loader.idleSprite("PB", 100);
    CHECK(pawn.is_loaded());
}

TEST_CASE("SpriteLoader caches: repeated calls for the same code don't reload")
{
    SpriteLoader loader(std::string(PROJECT_ROOT) + "/assets/pieces_classic");

    const Img &first = loader.idleSprite("KB", 100);
    const Img &second = loader.idleSprite("KB", 100);

    // Same cached object, not just equal content.
    CHECK(&first == &second);
}