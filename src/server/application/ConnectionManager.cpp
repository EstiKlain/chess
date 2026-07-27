#include "server/application/ConnectionManager.hpp"

#include "server/application/PlayerSessionRegistry.hpp"

namespace {

bool colorTaken(const std::unordered_map<std::string, ConnectionBinding>& connections, char color) {
    for (const auto& [id, binding] : connections) {
        if (binding.color == color) {
            return true;
        }
    }
    return false;
}

}  // namespace

ConnectionOutcome ConnectionManager::onConnected(const std::string& connectionId, GameSession* session,
                                                  const PlayerSessionRegistry& sessions) {
    if (!colorTaken(connections_, 'w') && !sessions.colorReserved(session, 'w')) {
        connections_[connectionId] = ConnectionBinding{session, 'w'};
        return ConnectionOutcome{true, 'w'};
    }
    if (!colorTaken(connections_, 'b') && !sessions.colorReserved(session, 'b')) {
        connections_[connectionId] = ConnectionBinding{session, 'b'};
        return ConnectionOutcome{true, 'b'};
    }
    return ConnectionOutcome{false, ' '};
}

void ConnectionManager::bindKnown(const std::string& connectionId, GameSession* session, char color) {
    connections_[connectionId] = ConnectionBinding{session, color};
}

void ConnectionManager::onDisconnected(const std::string& connectionId) {
    connections_.erase(connectionId);
}

std::optional<ConnectionBinding> ConnectionManager::sessionFor(const std::string& connectionId) const {
    auto it = connections_.find(connectionId);
    if (it == connections_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::pair<std::string, ConnectionBinding>> ConnectionManager::connectionsFor(
    const GameSession* session) const {
    std::vector<std::pair<std::string, ConnectionBinding>> result;
    for (const auto& [id, binding] : connections_) {
        if (binding.session == session) {
            result.emplace_back(id, binding);
        }
    }
    return result;
}

bool ConnectionManager::isConnected(const std::string& connectionId) const {
    return connections_.count(connectionId) > 0;
}

std::size_t ConnectionManager::connectionCount() const {
    return connections_.size();
}
