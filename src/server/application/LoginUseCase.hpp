#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

// Real LOGIN handling, kept out of main_server.cpp for the same reason
// MakeMoveUseCase is: the composition root stays wiring-only. Depends only
// on domain_ports/ + protocol/, never on WebSocketTransport/
// InProcessEventBus directly.
class LoginUseCase {
public:
    LoginUseCase(IIdentityStore& identities, ITransport& transport);

    /// Handles a LOGIN event: rejects a missing/empty username with MALFORMED_PAYLOAD, otherwise records the association and replies LOGIN_OK.
    void handleLogin(const std::string& connectionId, const std::string& requestId, const nlohmann::json& payload);

private:
    IIdentityStore& identities_;
    ITransport& transport_;
};
