#include "server/application/ConnectionManager.hpp"

#include <algorithm>

void ConnectionManager::bindKnown(const std::string& connectionId, GameSession* session, char color) {
    connections_[connectionId] = ConnectionBinding{session, color};
    bySession_[session].push_back(connectionId);
}

void ConnectionManager::onDisconnected(const std::string& connectionId) {
    const auto it = connections_.find(connectionId);
    if (it != connections_.end()) {
        auto& ids = bySession_[it->second.session];
        ids.erase(std::remove(ids.begin(), ids.end(), connectionId), ids.end());
    }
    connections_.erase(connectionId);
}

std::optional<ConnectionBinding> ConnectionManager::sessionFor(const std::string& connectionId) const {
    auto it = connections_.find(connectionId);
    if (it == connections_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void ConnectionManager::setElo(const std::string& connectionId, int elo) {
    const auto it = connections_.find(connectionId);
    if (it != connections_.end()) {
        it->second.elo = elo;
    }
}

std::vector<std::pair<std::string, ConnectionBinding>> ConnectionManager::connectionsFor(
    const GameSession* session) const {
    std::vector<std::pair<std::string, ConnectionBinding>> result;
    const auto it = bySession_.find(session);
    if (it == bySession_.end()) {
        return result;
    }
    result.reserve(it->second.size());
    for (const std::string& id : it->second) {
        result.emplace_back(id, connections_.at(id));
    }
    return result;
}

bool ConnectionManager::isConnected(const std::string& connectionId) const {
    return connections_.count(connectionId) > 0;
}

std::size_t ConnectionManager::connectionCount() const {
    return connections_.size();
}
