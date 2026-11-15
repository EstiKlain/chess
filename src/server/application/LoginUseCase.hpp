#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/IPasswordHasher.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/domain_ports/IUserRepository.hpp"

// Real LOGIN handling, kept out of main_server.cpp so the composition root
// stays wiring-only.
//
// Deliberately does NOT trigger a STATE_UPDATE broadcast itself - see
// ServerTick's unconditional per-tick broadcast, which already covers this.
class LoginUseCase {
public:
    LoginUseCase(IIdentityStore& identities, ITransport& transport, IUserRepository& users, IPasswordHasher& hasher);

    /// Handles a LOGIN event: rejects a missing/empty username or password with MALFORMED_PAYLOAD. Registers a brand-new username automatically (with the given password, hashed); for an existing username, rejects a wrong password with INVALID_CREDENTIALS without logging in. On success, records the identity association and replies LOGIN_OK with { username, elo } - no sessionToken here, since there is no seat yet (see PlayRequestUseCase/MatchStarter for where that gets minted).
    void handleLogin(const std::string& connectionId, const std::string& requestId, const nlohmann::json& payload);

private:
    IIdentityStore& identities_;
    ITransport& transport_;
    IUserRepository& users_;
    IPasswordHasher& hasher_;
};
