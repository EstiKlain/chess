#include "server/application/ConnectionManager.hpp"

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

ConnectionOutcome ConnectionManager::onConnected(const std::string& connectionId, GameSession* session) {
    if (!colorTaken(connections_, 'w')) {
        connections_[connectionId] = ConnectionBinding{session, 'w'};
        return ConnectionOutcome{true, 'w'};
    }
    if (!colorTaken(connections_, 'b')) {
        connections_[connectionId] = ConnectionBinding{session, 'b'};
        return ConnectionOutcome{true, 'b'};
    }
    return ConnectionOutcome{false, ' '};
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

std::vector<std::string> ConnectionManager::connectionIdsFor(const GameSession* session) const {
    std::vector<std::string> ids;
    for (const auto& [id, binding] : connections_) {
        if (binding.session == session) {
            ids.push_back(id);
        }
    }
    return ids;
}

bool ConnectionManager::isConnected(const std::string& connectionId) const {
    return connections_.count(connectionId) > 0;
}

std::size_t ConnectionManager::connectionCount() const {
    return connections_.size();
}
