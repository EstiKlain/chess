#include "server/infrastructure/persistence/InMemoryIdentityStore.hpp"

void InMemoryIdentityStore::login(const std::string& connectionId, const std::string& username) {
    names_[connectionId] = username;
}

bool InMemoryIdentityStore::isLoggedIn(const std::string& connectionId) const {
    return names_.count(connectionId) > 0;
}

std::optional<std::string> InMemoryIdentityStore::usernameFor(const std::string& connectionId) const {
    const auto it = names_.find(connectionId);
    if (it == names_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void InMemoryIdentityStore::logout(const std::string& connectionId) {
    names_.erase(connectionId);
}
