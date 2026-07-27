#pragma once

// kPort moved to shared/protocol/config.hpp - it's part of the wire
// protocol's connection contract, not server-internal configuration.
namespace server_config
{

    // How often the server's tick thread calls GameSession::wait() to advance
    // the game clock (resolve in-flight motions/cooldowns). This is the
    // server-side equivalent of chess_gui's render-loop frame delta - just
    // without any drawing, since rendering is the client's job.
    constexpr int kTickIntervalMs = 50;

    // How long a disconnected player's seat stays reserved before an
    // automatic resign (Server-Iteration 5). Checked via IClock::nowMs()
    // polling from the same tick thread, never a scheduled callback.
    constexpr long kReconnectWindowMs = 20000;

}
