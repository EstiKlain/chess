// Dedicated entry point for chess_server_tests. Kept separate from
// tests/unit/doctest_main.cpp (the core/GUI test target) per the plan's
// decision: server-layer tests run from their own target, never through
// chess_game.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
