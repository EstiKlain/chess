#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "client_net/domain_ports/IServerLink.hpp"
#include "engine/GameSnapshot.hpp"
#include "engine/MoveRequest.hpp"
#include "shared/protocol/dto/StateUpdateDto.hpp"

// The network requester Controller binds to in place of a local GameEngine:
// implements the same duck-typed interface (requestMove/requestJump)
// Controller's template constructor already expects, so Controller itself
// needs zero changes. Depends on IServerLink (a port, not a concrete
// websocketpp type) - this class owns only the LOGIN/STATE_UPDATE
// interpretation (envelope types, DTO<->GameSnapshot conversion, what
// "logged in" means), the same split as MakeMoveUseCase depending on
// ITransport rather than WebSocketTransport directly. Lets this class be
// unit tested with a fake link, never a real socket.
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

    /// Stops the underlying link.
    void stop();

private:
    void onMessage(const std::string& rawJson);
    void send(const std::string& type, const std::string& requestId, const nlohmann::json& payload);
    std::string nextRequestId();

    IServerLink& link_;

    mutable std::mutex mutex_;
    std::condition_variable snapshotCv_;
    GameSnapshot snapshot_;
    std::vector<PlayerDto> players_;
    bool hasSnapshot_ = false;
    std::optional<std::string> sessionToken_;
    std::optional<int> disconnectCountdownSeconds_;

    std::mutex loginMutex_;
    std::condition_variable loginCv_;
    bool loginPending_ = false;
    bool loginResult_ = false;
    std::string lastError_;

    int requestIdCounter_ = 0;
};
