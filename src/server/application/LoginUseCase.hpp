#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

// Real LOGIN handling, kept out of main_server.cpp so the composition root
// stays wiring-only. Depends only on domain_ports/ + protocol/, never on
// WebSocketTransport/InProcessEventBus directly.
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
    LoginUseCase(IIdentityStore& identities, ITransport& transport);

    /// Handles a LOGIN event: rejects a missing/empty username with MALFORMED_PAYLOAD, otherwise records the association and replies LOGIN_OK.
    void handleLogin(const std::string& connectionId, const std::string& requestId, const nlohmann::json& payload);

private:
    IIdentityStore& identities_;
    ITransport& transport_;
};
