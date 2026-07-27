#pragma once

#include <cstdint>

// The TCP port both chess_server and every chess_gui client agree on -
// part of the wire protocol's connection contract, not server-internal
// configuration (see server/config.hpp for that).
namespace server_config {

    constexpr uint16_t kPort = 9002;

}
