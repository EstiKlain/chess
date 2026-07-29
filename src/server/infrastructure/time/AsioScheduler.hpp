#pragma once

#include <asio.hpp>
#include <chrono>

#include "server/domain_ports/IScheduler.hpp"

// Real IScheduler adapter - runs the callback on whatever thread calls
// ioContext.run(), no thread of its own. Not unit-tested - real timer
// plumbing, same category as WebSocketTransport.
class AsioScheduler : public IScheduler {
public:
    explicit AsioScheduler(asio::io_context& ioContext) : ioContext_(ioContext) {}

    void schedulePeriodic(int intervalMs, std::function<void(long deltaMs)> callback) override;

private:
    void scheduleNext(int intervalMs, std::function<void(long deltaMs)> callback);

    asio::io_context& ioContext_;
    std::unique_ptr<asio::steady_timer> timer_;
    std::chrono::steady_clock::time_point lastTick_;
};
