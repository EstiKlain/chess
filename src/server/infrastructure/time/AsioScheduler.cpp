#include "server/infrastructure/time/AsioScheduler.hpp"

void AsioScheduler::schedulePeriodic(int intervalMs, std::function<void(long deltaMs)> callback) {
    jobs_.push_back(std::make_unique<Job>());
    Job& job = *jobs_.back();
    job.timer = std::make_unique<asio::steady_timer>(ioContext_);
    job.lastTick = std::chrono::steady_clock::now();
    scheduleNext(job, intervalMs, std::move(callback));
}

void AsioScheduler::scheduleNext(Job& job, int intervalMs, std::function<void(long deltaMs)> callback) {
    job.timer->expires_after(std::chrono::milliseconds(intervalMs));
    job.timer->async_wait([this, &job, intervalMs, callback](const asio::error_code& ec) {
        if (ec) {
            return;  // Timer cancelled (e.g. io_context stopping) - nothing to reschedule.
        }
        const auto now = std::chrono::steady_clock::now();
        const long deltaMs =
            static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(now - job.lastTick).count());
        job.lastTick = now;
        callback(deltaMs);
        scheduleNext(job, intervalMs, callback);
    });
}
