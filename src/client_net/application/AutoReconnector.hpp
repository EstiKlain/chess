#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

#include "client_net/application/ServerConnection.hpp"

// Owns the retry POLICY for reconnecting after a dropped connection - when
// to retry, how long, when to stop - the client-side mirror of the split
// already used server-side between DisconnectUseCase (policy) and
// ReconnectUseCase (the protocol of one attempt). ServerConnection::
// reconnect() is the one-attempt protocol operation (mirroring login()'s
// own shape); this class is the only thing that decides how many times to
// call it and for how long.
//
// A single persistent background thread, started once in the constructor -
// not one spawned per disconnect event - so a game with multiple, separate
// disconnects is handled cleanly with no thread-lifecycle churn.
class AutoReconnector {
public:
    explicit AutoReconnector(ServerConnection& connection);
    ~AutoReconnector();

    AutoReconnector(const AutoReconnector&) = delete;
    AutoReconnector& operator=(const AutoReconnector&) = delete;

    /// True while a reconnect episode is actively retrying.
    bool isReconnecting() const;

    /// True once an episode's local deadline passed without ever succeeding. Stays true until process exit - there is no UI path back from this state (see ReconnectingHud::drawConnectionLost).
    bool hasFailed() const;

    /// Seconds left in the current episode's local deadline, if one is in effect - nullopt when not reconnecting.
    std::optional<int> secondsRemaining() const;

private:
    void onDisconnected();
    void workerLoop();

    ServerConnection& connection_;
    std::thread worker_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool reconnectRequested_ = false;
    bool shuttingDown_ = false;
    std::chrono::steady_clock::time_point deadline_;

    std::atomic<bool> reconnecting_{false};
    std::atomic<bool> failed_{false};
};
