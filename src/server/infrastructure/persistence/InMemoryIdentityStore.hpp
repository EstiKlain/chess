#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "server/domain_ports/IIdentityStore.hpp"

// Concrete IIdentityStore: a process-local connectionId -> username map.
// Lives in infrastructure/persistence/ alongside the future Iteration-6
// SqliteUserRepository - "persistence" is the architectural term for "the
// layer that retains data beyond a single call", whether in-memory today
// or SQL-backed later; both fill the same port/role.
class InMemoryIdentityStore : public IIdentityStore {
public:
    void login(const std::string& connectionId, const std::string& username) override;
    bool isLoggedIn(const std::string& connectionId) const override;
    std::optional<std::string> usernameFor(const std::string& connectionId) const override;
    void logout(const std::string& connectionId) override;

private:
    std::unordered_map<std::string, std::string> names_;
};
