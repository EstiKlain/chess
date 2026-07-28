#include "client_net/application/AutoReconnector.hpp"

#include <cmath>

#include "shared/protocol/config.hpp"

namespace {
// How long to wait between failed reconnect attempts - named, not inline,
// per this project's convention for timing constants.
constexpr int kRetryIntervalMs = 1000;
}  // namespace

AutoReconnector::AutoReconnector(ServerConnection& connection) : connection_(connection) {
    connection_.setOnDisconnected([this] { onDisconnected(); });
    worker_ = std::thread([this] { workerLoop(); });
}

AutoReconnector::~AutoReconnector() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shuttingDown_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool AutoReconnector::isReconnecting() const {
    return reconnecting_.load();
}

bool AutoReconnector::hasFailed() const {
    return failed_.load();
}

std::optional<int> AutoReconnector::secondsRemaining() const {
    if (!reconnecting_.load()) {
        return std::nullopt;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    const auto remainingMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline_ - std::chrono::steady_clock::now()).count();
    return std::max(0, static_cast<int>(std::ceil(remainingMs / 1000.0)));
}

void AutoReconnector::onDisconnected() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        reconnectRequested_ = true;
    }
    cv_.notify_all();
}

void AutoReconnector::workerLoop() {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return reconnectRequested_ || shuttingDown_; });
        if (shuttingDown_) {
            return;
        }
        reconnectRequested_ = false;
        deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(server_config::kReconnectWindowMs);
        lock.unlock();

        reconnecting_ = true;
        failed_ = false;

        bool success = false;
        while (true) {
            {
                std::lock_guard<std::mutex> stopLock(mutex_);
                if (shuttingDown_) break;
                if (std::chrono::steady_clock::now() >= deadline_) break;
            }

            if (connection_.reconnect() == ReconnectOutcome::Success) {
                success = true;
                break;
            }

            std::unique_lock<std::mutex> sleepLock(mutex_);
            cv_.wait_for(sleepLock, std::chrono::milliseconds(kRetryIntervalMs), [this] { return shuttingDown_; });
            if (shuttingDown_) break;
        }

        reconnecting_ = false;
        failed_ = !success;
    }
}
