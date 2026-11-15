#pragma once

#include "client_net/application/ServerConnection.hpp"

/// Blocks the console until either a match is found, the user declines to search again after NO_MATCH_FOUND, or the account turns out to already have a game running (ALREADY_IN_GAME). Console-only - no window exists yet at this point. Returns false if the user chose to quit or the account is already in a game.
bool waitForMatchInteractively(ServerConnection& connection);
