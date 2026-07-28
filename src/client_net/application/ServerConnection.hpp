#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "client_net/application/SnapshotCache.hpp"
#include "client_net/domain_ports/IServerLink.hpp"
#include "engine/GameSnapshot.hpp"
#include "engine/MoveRequest.hpp"
#include "shared/protocol/dto/StateUpdateDto.hpp"

// The network requester Controller binds to in place of a local GameEngine:
// implements the same duck-typed interface (requestMove/requestJump)
// Controller's template constructor already expects, so Controller itself
// needs zero changes. Depends on IServerLink (a port, not a concrete
// websocketpp type) - this class owns protocol interpretation (envelope
// dispatch, DTO<->GameSnapshot conversion, the login/reconnect state
// machines - what "logged in" and "reconnected" mean), the same split as
// MakeMoveUseCase depending on ITransport rather than WebSocketTransport
// directly. Lets this class be unit tested with a fake link, never a real
// socket. Deliberately does NOT own the "what should currently be
// displayed" read-model itself (snapshot/players/sessionToken/disconnect
// countdown) - that lives in a separate SnapshotCache member, a distinct
// responsibility (this class interprets the wire, SnapshotCache remembers
// what was last interpreted for the render loop to read).
/// Outcome of a single reconnect() attempt. Deliberately two-valued, not
/// three: an early SESSION_EXPIRED can be indistinguishable from a
/// genuinely expired token if the server hasn't finished marking the
/// connection disconnected yet (a real race between client-side and
/// server-side disconnect detection) - so nothing about a single reply ever
/// causes a permanent give-up. Only AutoReconnector's own wall-clock
/// deadline (shared with the server via server_config::kReconnectWindowMs)
/// ever ends a reconnect episode without Success.
enum class ReconnectOutcome { Success, Retry };

class ServerConnection {
public:
    explicit ServerConnection(IServerLink& link);

    /// Opens the connection to host:port and blocks until the handshake completes. Throws std::runtime_error on failure.
    void connect(const std::string& host, uint16_t port);

    /// Sends LOGIN and blocks until LOGIN_OK or ERROR arrives. Returns true on LOGIN_OK; on false, lastError() holds the server's message.
    bool login(const std::string& username);

    /// The server's error message from the most recent failed login, if any.
    const std::string& lastError() const;

    /// Blocks until at least one STATE_UPDATE has been received (the server's tick loop broadcasts unconditionally, so this resolves within one tick interval of a successful login). Used to learn the real board dimensions before constructing anything geometry-dependent, instead of assuming a fixed board size anywhere in client code.
    GameSnapshot awaitInitialSnapshot();

    /// Controller-compatible requester interface - matches Controller's template constructor exactly.
    void requestMove(const MoveRequest& request);
    void requestJump(int row, int col);

    /// Returns a copy of the most recently received snapshot/roster. Thread-safe (mutex-guarded) - whatever thread the link delivers messages on writes, the render-loop thread reads, same defensive pattern already used by GameSession for the analogous cross-thread problem.
    GameSnapshot latestSnapshot() const;
    std::vector<PlayerDto> latestPlayers() const;

    /// The sessionToken received in LOGIN_OK's payload (Server-Iteration 5), if login has succeeded - nullopt otherwise. Not yet consumed by anything in this class; stored so a future RECONNECT flow doesn't need to touch LOGIN_OK handling again.
    std::optional<std::string> sessionToken() const;

    /// Seconds remaining before the opponent auto-resigns, per the most recent DISCONNECT_COUNTDOWN - nullopt when no countdown is in effect (never started, cancelled by a successful reconnect, or the game is already over).
    std::optional<int> latestDisconnectCountdown() const;

    /// One reconnect attempt: redials the link (host/port remembered from the original connect() call), sends RECONNECT with the stored sessionToken, and waits (bounded) for the matching reply. No retry policy here - that's AutoReconnector's job, mirroring how login() is also a single blocking attempt. Returns Retry if there is no sessionToken to reconnect with at all (nothing logged in yet).
    ReconnectOutcome reconnect();

    /// Registers a handler to be invoked when the underlying link dies unexpectedly (forwarded from IServerLink::setOnClose) - but only once we've actually logged in at least once (nothing meaningful to reconnect to otherwise). Also clears latestDisconnectCountdown() first: a countdown about the opponent becomes meaningless the moment our own link goes down too.
    void setOnDisconnected(std::function<void()> handler);

    /// Stops the underlying link.
    void stop();

private:
    void onMessage(const std::string& rawJson);
    void send(const std::string& type, const std::string& requestId, const nlohmann::json& payload);
    std::string nextRequestId();

    IServerLink& link_;
    SnapshotCache cache_;

    std::mutex loginMutex_;
    std::condition_variable loginCv_;
    bool loginPending_ = false;
    bool loginResult_ = false;
    std::string lastError_;

    std::string host_;
    uint16_t port_ = 0;

    // Correlates a RECONNECT attempt's reply by requestId, not a plain
    // pending/result flag pair - a late reply from a previous, already
    // timed-out attempt must not resolve a later attempt's wait. Guarded by
    // its own mutex, independent of both loginMutex_ and cache_'s internal
    // one (a RECONNECT reply arrives as an ordinary STATE_UPDATE/ERROR
    // through onMessage, handled by one more small, independent check in
    // each of those branches).
    std::mutex reconnectReplyMutex_;
    std::condition_variable reconnectReplyCv_;
    std::string pendingReconnectRequestId_;
    bool reconnectReplySuccess_ = false;

    // Atomic: called from both the render/main thread (login/requestMove/
    // requestJump) and AutoReconnector's background worker thread (via
    // reconnect()) - a plain int here would be a data race.
    std::atomic<int> requestIdCounter_{0};
};
