#include "client_net/application/ServerConnection.hpp"

#include <utility>

#include "server/protocol/Envelope.hpp"
#include "server/protocol/dto/JumpDto.hpp"
#include "server/protocol/dto/MessageEnvelope.hpp"
#include "server/protocol/dto/MoveDto.hpp"
#include "server/protocol/mappers/GameSnapshotMapper.hpp"
#include "server/protocol/mappers/MoveRequestMapper.hpp"

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
            }
            snapshotCv_.notify_all();
        } catch (const std::exception&) {
            // Malformed payload - ignore, keep the last good snapshot.
        }
        return;
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
