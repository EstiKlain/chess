#pragma once

// Port: application code depends on THIS, never on std::chrono or a
// concrete clock directly. Deliberately polling-only - mirrors
// GameEngine::wait(ms)'s own poll-and-check shape (no scheduling/callback/
// timer-registration method exists here on purpose). DisconnectUseCase is
// ticked from the server's existing periodic tick thread, exactly like
// GameSession::wait already is; a fake implementation for tests only needs
// to support manual advancement of nowMs(), nothing event-driven.
class IClock {
public:
    virtual ~IClock() = default;

    /// Milliseconds elapsed on some monotonic reference the caller never
    /// needs to interpret as wall-clock/calendar time - only differences
    /// between two calls are meaningful.
    virtual long nowMs() const = 0;
};
