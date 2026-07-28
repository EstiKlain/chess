#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "server/application/ConnectionManager.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/IPasswordHasher.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/domain_ports/IUserRepository.hpp"

// Real LOGIN handling, kept out of main_server.cpp so the composition root
// stays wiring-only. Depends only on domain_ports/ + protocol/ + application/
// (ConnectionManager), never on WebSocketTransport/InProcessEventBus
// directly.
//
// Deliberately does NOT trigger a STATE_UPDATE broadcast itself - it did,
// briefly, during Iteration 3.5 (so a freshly-logged-in client wouldn't
// wait for the first move to see the board), but that became redundant
// once the server's tick thread (main_server.cpp) started broadcasting
// unconditionally every tick (server_config::kTickIntervalMs, currently
// 50ms) - any connection, logged in or not yet moved, is picked up by the
// very next tick regardless. Keeping both would be two mechanisms solving
// the same problem.
class LoginUseCase {
public:
    LoginUseCase(IIdentityStore& identities, ITransport& transport, ConnectionManager& connections,
                 PlayerSessionRegistry& sessions, IUserRepository& users, IPasswordHasher& hasher);

    /// Handles a LOGIN event: rejects a connection with no session binding (e.g. a 3rd/rejected connection - see ConnectionManager::onConnected) with TABLE_FULL, rejects a missing/empty username or password with MALFORMED_PAYLOAD. Registers a brand-new username automatically (with the given password, hashed); for an existing username, rejects a wrong password with INVALID_CREDENTIALS without logging in. On success, records the identity association, caches the account's current Elo on the connection (ConnectionManager::setElo), registers a fresh sessionToken (Server-Iteration 5, for later RECONNECT), and replies LOGIN_OK with { sessionToken }.
    void handleLogin(const std::string& connectionId, const std::string& requestId, const nlohmann::json& payload);

private:
    IIdentityStore& identities_;
    ITransport& transport_;
    ConnectionManager& connections_;
    PlayerSessionRegistry& sessions_;
    IUserRepository& users_;
    IPasswordHasher& hasher_;
};
