#pragma once

#include <string>

#include "server/application/ConnectionManager.hpp"
#include "server/application/GameSession.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

namespace StateFanOut {

/// Sends STATE_UPDATE (each recipient's own role, plus the shared players[] roster) to every connection bound to session. originConnectionId/originRequestId identify whose request (if any) triggered this broadcast - only that connection gets its requestId echoed back, everyone else gets "". Pass ("", "") when the broadcast isn't a reply to any specific request (e.g. triggered by a login, not a move).
void broadcast(GameSession& session, ConnectionManager& connections, IIdentityStore& identities,
                ITransport& transport, const std::string& originConnectionId, const std::string& originRequestId);

}  // namespace StateFanOut
