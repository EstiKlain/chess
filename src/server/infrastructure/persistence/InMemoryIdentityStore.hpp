#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "server/domain_ports/IIdentityStore.hpp"

// Concrete IIdentityStore: a process-local connectionId -> username map.
// Lives in infrastructure/persistence/ alongside Iteration 6's
// InMemoryUserRepository/PostgresUserRepository - "persistence" is the
// architectural term for "the layer that retains data beyond a single
// call", whether in-memory today or SQL-backed later; both fill the same
// port/role. Deliberately stays separate from IUserRepository (Iteration
// 6) rather than being replaced by it - this port is a fast, connection-
// scoped cache with no credential concept, read unconditionally every tick
// by StateFanOut; IUserRepository is durable, credential-related, and only
// ever consulted inside LoginUseCase.
class InMemoryIdentityStore : public IIdentityStore {
public:
    void login(const std::string& connectionId, const std::string& username) override;
    bool isLoggedIn(const std::string& connectionId) const override;
    std::optional<std::string> usernameFor(const std::string& connectionId) const override;
    void logout(const std::string& connectionId) override;

private:
    std::unordered_map<std::string, std::string> names_;
};
