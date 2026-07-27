#pragma once

#include <chrono>

#include "server/domain_ports/IClock.hpp"

// Real IClock adapter, wrapping steady_clock. Anchored at construction
// time (like GameEngine's own elapsedMs_), not at the epoch - nowMs() only
// needs to support differences, so there is no reason to carry a huge
// since-epoch value around.
class SystemClock : public IClock {
public:
    SystemClock() : start_(std::chrono::steady_clock::now()) {}

    long nowMs() const override;

private:
    std::chrono::steady_clock::time_point start_;
};
