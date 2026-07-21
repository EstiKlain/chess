#include "server/application/ConnectionManager.hpp"

void ConnectionManager::onConnected(const std::string& connectionId) {
    connections_.insert(connectionId);
}

void ConnectionManager::onDisconnected(const std::string& connectionId) {
    connections_.erase(connectionId);
}

bool ConnectionManager::isConnected(const std::string& connectionId) const {
    return connections_.count(connectionId) > 0;
}

std::size_t ConnectionManager::connectionCount() const {
    return connections_.size();
}
