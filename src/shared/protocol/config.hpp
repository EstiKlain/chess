#pragma once

#include <cstdint>

// The TCP port both chess_server and every chess_gui client agree on -
// part of the wire protocol's connection contract, not server-internal
// configuration (see server/config.hpp for that).
namespace server_config {

    constexpr uint16_t kPort = 9002;

    // How long a disconnected player's seat stays reserved before an
    // automatic resign (Server-Iteration 5) - the server enforces this via
    // DisconnectUseCase; the client's AutoReconnector (Iteration 5 follow-up)
    // needs the same number to know how long it's worth retrying a dropped
    // connection. A wire-contract value both sides must agree on, same
    // category as kPort - not server-internal config.
    constexpr long kReconnectWindowMs = 20000;

}
