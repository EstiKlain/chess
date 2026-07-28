#include "server/application/PlayerSessionRegistry.hpp"

PlayerSessionRegistry::PlayerSessionRegistry(ITokenGenerator& tokenGenerator) : tokenGenerator_(tokenGenerator) {}

std::string PlayerSessionRegistry::registerNew(const std::string& username, char color, GameSession* session,
                                                const std::string& connectionId) {
    const std::string token = tokenGenerator_.generate();
    byToken_[token] = PlayerSessionEntry{username, color, session, connectionId, std::nullopt};
    return token;
}

std::optional<PlayerSessionEntry> PlayerSessionRegistry::find(const std::string& token) const {
    const auto it = byToken_.find(token);
    if (it == byToken_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void PlayerSessionRegistry::markDisconnected(const std::string& connectionId, long nowMs) {
    for (auto& [token, entry] : byToken_) {
        if (entry.connectionId == connectionId) {
            entry.connectionId = std::nullopt;
            entry.disconnectedAtMs = nowMs;
            return;
        }
    }
}

bool PlayerSessionRegistry::reconnect(const std::string& token, const std::string& newConnectionId) {
    auto it = byToken_.find(token);
    if (it == byToken_.end() || it->second.connectionId.has_value()) {
        return false;
    }
    it->second.connectionId = newConnectionId;
    it->second.disconnectedAtMs = std::nullopt;
    return true;
}

void PlayerSessionRegistry::remove(const std::string& token) { byToken_.erase(token); }

bool PlayerSessionRegistry::colorReserved(const GameSession* session, char color) const {
    for (const auto& [token, entry] : byToken_) {
        if (entry.session == session && entry.color == color) {
            return true;
        }
    }
    return false;
}

std::vector<std::pair<std::string, PlayerSessionEntry>> PlayerSessionRegistry::disconnectedEntries() const {
    std::vector<std::pair<std::string, PlayerSessionEntry>> result;
    for (const auto& [token, entry] : byToken_) {
        if (!entry.connectionId.has_value()) {
            result.emplace_back(token, entry);
        }
    }
    return result;
}
