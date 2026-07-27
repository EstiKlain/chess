#include "server/infrastructure/time/SystemClock.hpp"

long SystemClock::nowMs() const {
    return static_cast<long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_).count());
}
