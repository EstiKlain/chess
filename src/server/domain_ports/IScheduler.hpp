#pragma once

#include <functional>

// Port: application code depends on THIS, never on asio/a timer directly.
// Separate from IClock (which is deliberately polling-only, "what time is
// it") and from ITransport (which only ever means "send/receive bytes") -
// periodic scheduling is a third, distinct responsibility, one port per
// responsibility like everywhere else in this codebase.
class IScheduler {
public:
    virtual ~IScheduler() = default;

    /// Invokes callback every intervalMs, forever, until the scheduler is destroyed.
    /// deltaMs is the real time elapsed since the previous invocation (or since
    /// scheduling started, for the first one) - callers use it exactly like the
    /// tick-thread's own deltaMs was used before this port existed.
    virtual void schedulePeriodic(int intervalMs, std::function<void(long deltaMs)> callback) = 0;
};
