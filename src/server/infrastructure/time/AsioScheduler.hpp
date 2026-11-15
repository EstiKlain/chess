#pragma once

#include <asio.hpp>
#include <chrono>
#include <memory>
#include <vector>

#include "server/domain_ports/IScheduler.hpp"

// Real IScheduler adapter - runs callbacks on whatever thread calls
// ioContext.run(), no thread of its own. Not unit-tested - real timer
// plumbing, same category as WebSocketTransport.
class AsioScheduler : public IScheduler {
public:
    explicit AsioScheduler(asio::io_context& ioContext) : ioContext_(ioContext) {}

    void schedulePeriodic(int intervalMs, std::function<void(long deltaMs)> callback) override;

private:
    // Per-registration state: schedulePeriodic can be called any number of
    // times on the same AsioScheduler, and each call needs its own timer -
    // sharing one timer_ member across calls would cancel an earlier
    // registration's pending async_wait the moment a later call replaces it.
    struct Job {
        std::unique_ptr<asio::steady_timer> timer;
        std::chrono::steady_clock::time_point lastTick;
    };

    void scheduleNext(Job& job, int intervalMs, std::function<void(long deltaMs)> callback);

    asio::io_context& ioContext_;
    // unique_ptr<Job>, not Job by value: the async_wait lambda captures &job,
    // so its address must stay stable even as jobs_ grows and reallocates.
    std::vector<std::unique_ptr<Job>> jobs_;
};
