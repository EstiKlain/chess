#include "client_net/application/ServerConnection.hpp"

#include <utility>

#include "server/protocol/Envelope.hpp"
#include "shared/protocol/dto/JumpDto.hpp"
#include "shared/protocol/dto/MessageEnvelope.hpp"
#include "shared/protocol/dto/MoveDto.hpp"
#include "shared/protocol/mappers/GameSnapshotMapper.hpp"
#include "shared/protocol/mappers/MoveRequestMapper.hpp"

ServerConnection::ServerConnection(IServerLink& link) : link_(link) {
    link_.setOnMessage([this](const std::string& rawJson) { onMessage(rawJson); });
}

void ServerConnection::connect(const std::string& host, uint16_t port) {
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
    std::unique_lock<std::mutex> lock(mutex_);
    snapshotCv_.wait(lock, [this] { return hasSnapshot_; });
    return snapshot_;
}

void ServerConnection::requestMove(const MoveRequest& request) {
    send("MOVE", nextRequestId(), MoveRequestMapper::toDto(request));
}

void ServerConnection::requestJump(int row, int col) {
    send("JUMP", nextRequestId(), JumpDto{row, col});
}

GameSnapshot ServerConnection::latestSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

std::vector<PlayerDto> ServerConnection::latestPlayers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return players_;
}

std::optional<std::string> ServerConnection::sessionToken() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessionToken_;
}

std::optional<int> ServerConnection::latestDisconnectCountdown() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return disconnectCountdownSeconds_;
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
            {
                std::lock_guard<std::mutex> lock(mutex_);
                snapshot_ = std::move(newSnapshot);
                players_ = dto.players;
                hasSnapshot_ = true;
                // A disconnect countdown only ever makes sense while the game
                // is still ongoing - once gameOver is true (whether from a
                // king capture or an auto-resign timeout), any stale
                // countdown value must not keep being displayed.
                if (snapshot_.gameOver) {
                    disconnectCountdownSeconds_ = std::nullopt;
                }
            }
            snapshotCv_.notify_all();
        } catch (const std::exception&) {
            // Malformed payload - ignore, keep the last good snapshot.
        }
        return;
    }

    if (envelope.type == "DISCONNECT_COUNTDOWN") {
        try {
            const int secondsLeft = envelope.payload.value("secondsLeft", 0);
            std::lock_guard<std::mutex> lock(mutex_);
            // secondsLeft <= 0 is the server's explicit "the countdown is
            // over" signal (ReconnectUseCase sends this on a successful
            // reconnect) - clear rather than display a non-positive number.
            disconnectCountdownSeconds_ = (secondsLeft > 0) ? std::optional<int>(secondsLeft) : std::nullopt;
        } catch (const std::exception&) {
            // Malformed payload (e.g. secondsLeft not an integer) - ignore,
            // keep whatever countdown state was already in effect.
        }
        return;
    }

    if (envelope.type == "LOGIN_OK") {
        // Independent of the loginMutex_-guarded block below - this only
        // ever reads the sessionToken field, guarded by mutex_ alone, never
        // nested with loginMutex_.
        try {
            if (envelope.payload.contains("sessionToken")) {
                const std::string token = envelope.payload.at("sessionToken").get<std::string>();
                std::lock_guard<std::mutex> lock(mutex_);
                sessionToken_ = token;
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
}

std::string ServerConnection::nextRequestId() {
    return std::to_string(++requestIdCounter_);
}

void ServerConnection::send(const std::string& type, const std::string& requestId, const nlohmann::json& payload) {
    link_.send(protocol::envelope(type, requestId, payload));
}
