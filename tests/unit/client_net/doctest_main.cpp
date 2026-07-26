// Dedicated entry point for chess_client_tests. Kept separate from
// tests/unit/doctest_main.cpp (core/GUI) and tests/unit/server/doctest_main.cpp
// (server layer) - client_net/ has its own dependency footprint (no OpenCV,
// no websocketpp/asio - it's tested through the IServerLink fake, never a
// real socket) and deserves its own target rather than blurring into either.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
