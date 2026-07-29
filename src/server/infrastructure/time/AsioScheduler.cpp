#include "server/infrastructure/time/AsioScheduler.hpp"

void AsioScheduler::schedulePeriodic(int intervalMs, std::function<void(long deltaMs)> callback) {
    lastTick_ = std::chrono::steady_clock::now();
    timer_ = std::make_unique<asio::steady_timer>(ioContext_);
    scheduleNext(intervalMs, std::move(callback));
}

void AsioScheduler::scheduleNext(int intervalMs, std::function<void(long deltaMs)> callback) {
    timer_->expires_after(std::chrono::milliseconds(intervalMs));
    timer_->async_wait([this, intervalMs, callback](const asio::error_code& ec) {
        if (ec) {
            return;  // Timer cancelled (e.g. io_context stopping) - nothing to reschedule.
        }
        const auto now = std::chrono::steady_clock::now();
        const long deltaMs =
            static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count());
        lastTick_ = now;
        callback(deltaMs);
        scheduleNext(intervalMs, callback);
    });
}
