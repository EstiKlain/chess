#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "server/application/ConnectionManager.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

// Handles RECONNECT: reclaims a disconnected player's seat if their
// sessionToken is still valid (PlayerSessionRegistry::reconnect enforces
// "token known AND currently disconnected" as one precondition, which also
// rejects a double-reconnect race on the same token). RECONNECT is exempt
// from AuthGuard's login check (like LOGIN/PING) since the new connectionId
// was never logged in - this use case itself calls identities.login(...) on
// success, the same call LoginUseCase makes, so every later MOVE/JUMP from
// the new connectionId passes AuthGuard automatically afterward.
class ReconnectUseCase {
public:
    ReconnectUseCase(PlayerSessionRegistry& sessions, ConnectionManager& connections, IIdentityStore& identities,
                      ITransport& transport);

    /// Handles a RECONNECT event: SESSION_EXPIRED for an unknown token or one that is no longer in a disconnected state (window already elapsed and resigned, or a second reconnect racing an already-succeeded first one - same client-facing meaning, no distinction needed). Otherwise re-binds the connection to the same session/color, re-establishes login, and sends a fresh STATE_UPDATE to this connection only (not a full broadcast - the opponent's view is already current).
    void handleReconnect(const std::string& connectionId, const std::string& requestId, const nlohmann::json& payload);

private:
    PlayerSessionRegistry& sessions_;
    ConnectionManager& connections_;
    IIdentityStore& identities_;
    ITransport& transport_;
};
