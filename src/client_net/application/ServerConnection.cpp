#include "client_net/application/ServerConnection.hpp"

#include <chrono>
#include <utility>

#include "server/protocol/Envelope.hpp"
#include "shared/protocol/dto/JumpDto.hpp"
#include "shared/protocol/dto/MessageEnvelope.hpp"
#include "shared/protocol/dto/MoveDto.hpp"
#include "shared/protocol/mappers/GameSnapshotMapper.hpp"
#include "shared/protocol/mappers/MoveRequestMapper.hpp"

namespace {
// Bounds the wait for RECONNECT's reply (STATE_UPDATE on success, ERROR on
// failure) - a reply that never arrives (server accepted the TCP connection
// but never answered) must not hang the whole retry loop, exactly the same
// reasoning as WebSocketClientLink's own connect-handshake timeout.
constexpr std::chrono::milliseconds kReconnectReplyTimeout{3000};
}  // namespace

ServerConnection::ServerConnection(IServerLink& link) : link_(link) {
    link_.setOnMessage([this](const std::string& rawJson) { onMessage(rawJson); });
}

void ServerConnection::connect(const std::string& host, uint16_t port) {
    host_ = host;
    port_ = port;
    link_.connect(host, port);
}

bool ServerConnection::login(const std::string& username) {
    const std::string requestId = nextRequestId();
    {
        std::lock_guard<std::mutex> lock(loginMutex_);
        loginPending_ = true;
    }

    send("LOGIN", requestId, nlohmann::json{{"username", username}});

    std::unique_lock<std::mutex> lock(loginMutex_);
    loginCv_.wait(lock, [this] { return !loginPending_; });
    return loginResult_;
}

const std::string& ServerConnection::lastError() const {
    return lastError_;
}

GameSnapshot ServerConnection::awaitInitialSnapshot() {
    return cache_.awaitInitialSnapshot();
}

void ServerConnection::requestMove(const MoveRequest& request) {
    send("MOVE", nextRequestId(), MoveRequestMapper::toDto(request));
}

void ServerConnection::requestJump(int row, int col) {
    send("JUMP", nextRequestId(), JumpDto{row, col});
}

GameSnapshot ServerConnection::latestSnapshot() const {
    return cache_.latestSnapshot();
}

std::vector<PlayerDto> ServerConnection::latestPlayers() const {
    return cache_.latestPlayers();
}

std::optional<std::string> ServerConnection::sessionToken() const {
    return cache_.sessionToken();
}

std::optional<int> ServerConnection::latestDisconnectCountdown() const {
    return cache_.latestDisconnectCountdown();
}

ReconnectOutcome ServerConnection::reconnect() {
    const auto token = sessionToken();
    if (!token.has_value()) {
        return ReconnectOutcome::Retry;  // never logged in - nothing to reconnect to
    }

    try {
        link_.connect(host_, port_);
    } catch (const std::exception&) {
        return ReconnectOutcome::Retry;
    }

    const std::string requestId = nextRequestId();
    {
        std::lock_guard<std::mutex> lock(reconnectReplyMutex_);
        pendingReconnectRequestId_ = requestId;
    }
    send("RECONNECT", requestId, nlohmann::json{{"sessionToken", *token}});

    std::unique_lock<std::mutex> lock(reconnectReplyMutex_);
    const bool gotReply = reconnectReplyCv_.wait_for(lock, kReconnectReplyTimeout,
                                                      [this] { return pendingReconnectRequestId_.empty(); });
    if (!gotReply) {
        return ReconnectOutcome::Retry;
    }
    return reconnectReplySuccess_ ? ReconnectOutcome::Success : ReconnectOutcome::Retry;
}

void ServerConnection::setOnDisconnected(std::function<void()> handler) {
    link_.setOnClose([this, handler = std::move(handler)]() {
        cache_.clearDisconnectCountdown();
        if (cache_.sessionToken().has_value()) {
            handler();
        }
    });
}

void ServerConnection::stop() {
    link_.stop();
}

void ServerConnection::onMessage(const std::string& rawJson) {
    MessageEnvelope envelope;
    try {
        envelope = nlohmann::json::parse(rawJson).get<MessageEnvelope>();
    } catch (const std::exception&) {
        return;  // Malformed message from the server - nothing sensible to do but drop it.
    }

    if (envelope.type == "STATE_UPDATE") {
        try {
            const StateUpdateDto dto = envelope.payload.get<StateUpdateDto>();
            GameSnapshot newSnapshot = GameSnapshotMapper::fromDto(dto);
            cache_.updateFromStateUpdate(std::move(newSnapshot), dto.players);
        } catch (const std::exception&) {
            // Malformed payload - ignore, keep the last good snapshot.
        }

        // Independent of the snapshot-parsing try/catch above: a RECONNECT
        // attempt's success reply is an ordinary STATE_UPDATE, correlated by
        // requestId (checked regardless of whether the payload itself
        // parsed - the id is a top-level envelope field, not part of the
        // possibly-malformed payload).
        {
            std::lock_guard<std::mutex> lock(reconnectReplyMutex_);
            if (!pendingReconnectRequestId_.empty() && pendingReconnectRequestId_ == envelope.requestId) {
                pendingReconnectRequestId_.clear();
                reconnectReplySuccess_ = true;
                reconnectReplyCv_.notify_all();
            }
        }
        return;
    }

    if (envelope.type == "DISCONNECT_COUNTDOWN") {
        try {
            const int secondsLeft = envelope.payload.value("secondsLeft", 0);
            cache_.updateDisconnectCountdown(secondsLeft);
        } catch (const std::exception&) {
            // Malformed payload (e.g. secondsLeft not an integer) - ignore,
            // keep whatever countdown state was already in effect.
        }
        return;
    }

    if (envelope.type == "LOGIN_OK") {
        // Independent of the loginMutex_-guarded block below - this only
        // ever touches the sessionToken cache, never nested with loginMutex_.
        try {
            if (envelope.payload.contains("sessionToken")) {
                cache_.setSessionToken(envelope.payload.at("sessionToken").get<std::string>());
            }
        } catch (const std::exception&) {
            // Malformed sessionToken field - ignore, keep whatever token (if any) was already stored.
        }
    }

    if (envelope.type == "LOGIN_OK" || envelope.type == "ERROR") {
        std::lock_guard<std::mutex> lock(loginMutex_);
        if (loginPending_) {
            loginResult_ = (envelope.type == "LOGIN_OK");
            if (!loginResult_) {
                lastError_ = envelope.payload.value("message", std::string("login failed"));
            }
            loginPending_ = false;
            loginCv_.notify_all();
        }
        // Else: an ERROR unrelated to a pending login (e.g. ILLEGAL_MOVE) -
        // no UI surface for per-move errors in this iteration, so it's
        // silently dropped; the next STATE_UPDATE remains the source of
        // truth for what the board actually looks like.
    }

    if (envelope.type == "ERROR") {
        // Independent of the loginMutex_-guarded block above - a RECONNECT
        // attempt's failure reply (e.g. SESSION_EXPIRED) is just another
        // ERROR, correlated by requestId like the STATE_UPDATE success case.
        std::lock_guard<std::mutex> lock(reconnectReplyMutex_);
        if (!pendingReconnectRequestId_.empty() && pendingReconnectRequestId_ == envelope.requestId) {
            pendingReconnectRequestId_.clear();
            reconnectReplySuccess_ = false;
            reconnectReplyCv_.notify_all();
        }
    }
}

std::string ServerConnection::nextRequestId() {
    return std::to_string(++requestIdCounter_);
}

void ServerConnection::send(const std::string& type, const std::string& requestId, const nlohmann::json& payload) {
    link_.send(protocol::envelope(type, requestId, payload));
}
