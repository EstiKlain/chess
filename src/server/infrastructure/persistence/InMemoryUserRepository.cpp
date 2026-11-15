#include "server/infrastructure/persistence/InMemoryUserRepository.hpp"

#include "server/config.hpp"

std::optional<UserRecord> InMemoryUserRepository::findByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = users_.find(username);
    if (it == users_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void InMemoryUserRepository::createUser(const std::string& username, const std::string& passwordHash) {
    std::lock_guard<std::mutex> lock(mutex_);
    users_[username] = UserRecord{username, passwordHash, server_config::kInitialElo};
}

void InMemoryUserRepository::updateElo(const std::string& username, int newElo) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = users_.find(username);
    if (it != users_.end()) {
        it->second.elo = newElo;
    }
}
