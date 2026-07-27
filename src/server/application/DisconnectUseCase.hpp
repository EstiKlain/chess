#pragma once

#include <string>
#include <unordered_map>

#include "server/application/ConnectionManager.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/domain_ports/IClock.hpp"
#include "server/domain_ports/ITransport.hpp"

// Owns the disconnect half of reconnect logic (Server-Iteration 5):
// recognizing a dropped connection, counting down the reconnect window, and
// resigning automatically if nobody reclaims the seat in time. Reuses the
// server's existing periodic tick thread (main_server.cpp) rather than any
// scheduling infrastructure - see IClock's own doc comment.
class DisconnectUseCase {
public:
    DisconnectUseCase(PlayerSessionRegistry& sessions, ConnectionManager& connections, ITransport& transport,
                       IClock& clock);

    /// Wired from transport.setOnClose. Marks the player's registry entry as disconnected (recording clock_.nowMs()) if this connectionId belongs to a registered player; a no-op otherwise (e.g. a rejected/never-logged-in connection closing).
    void onDisconnected(const std::string& connectionId);

    /// Wired from the server's existing periodic tick thread, alongside GameSession::wait(deltaMs) - and, critically, AFTER it: wait() resolves a same-tick king-capture first, so a same-tick resign() (below) correctly refuses to overwrite it. For every currently-disconnected entry: sends DISCONNECT_COUNTDOWN to the opponent only when the integer secondsLeft changes; once kReconnectWindowMs has elapsed, resigns the seat and removes the registry entry.
    void tick(long nowMs);

private:
    PlayerSessionRegistry& sessions_;
    ConnectionManager& connections_;
    ITransport& transport_;
    IClock& clock_;

    // Per-token last-sent secondsLeft, so DISCONNECT_COUNTDOWN only sends on
    // change (~20 messages over the window, not one per 50ms tick). Pruned
    // every tick() call for tokens no longer disconnected (reconnected or
    // resigned-and-removed), so this never grows unboundedly.
    std::unordered_map<std::string, int> lastSentSecondsLeft_;
};
