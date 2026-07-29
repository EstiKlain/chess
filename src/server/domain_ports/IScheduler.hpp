#pragma once

#include <functional>

// Separate from IClock (polling-only) and ITransport (bytes only) - periodic
// scheduling is its own responsibility.
class IScheduler {
public:
    virtual ~IScheduler() = default;

    /// Invokes callback every intervalMs. deltaMs is the real time elapsed since the previous invocation.
    virtual void schedulePeriodic(int intervalMs, std::function<void(long deltaMs)> callback) = 0;
};
