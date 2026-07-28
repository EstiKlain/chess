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

    // Elo K-factor: how much a single game's result can move a rating.
    constexpr int kEloKFactor = 32;

    // Starting rating for a brand-new account. Also hardcoded as the SQL
    // DEFAULT in PostgresUserRepository's schema (SQL can't reference a C++
    // constant) - keep both in sync by hand if this ever changes.
    constexpr int kInitialElo = 1200;

}
