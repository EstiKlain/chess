#pragma once

#include <asio.hpp>
#include <chrono>

#include "server/domain_ports/IScheduler.hpp"

// Real IScheduler adapter. Runs the periodic callback on whatever thread
// calls ioContext.run() (main_server.cpp's main thread, via
// WebSocketTransport::run) - there is no thread of its own, which is the
// whole point: this is what lets the tick loop and message handling share
// a single thread instead of racing on ConnectionManager/
// PlayerSessionRegistry from two threads (see CLAUDE.md's Iteration 7
// threading decision).
//
// Not unit-tested - real IO/timer plumbing, same category as
// WebSocketTransport itself (not linked into chess_server_tests).
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
