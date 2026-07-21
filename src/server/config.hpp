#pragma once

#include <cstdint>

namespace server_config
{

    constexpr uint16_t kPort = 9002;

    // How often the server's tick thread calls GameSession::wait() to advance
    // the game clock (resolve in-flight motions/cooldowns). This is the
    // server-side equivalent of chess_gui's render-loop frame delta - just
    // without any drawing, since rendering is the client's job.
    constexpr int kTickIntervalMs = 50;

}
