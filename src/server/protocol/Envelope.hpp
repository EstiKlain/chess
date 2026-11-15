#pragma once

#include <string>

#include "server/domain_ports/ITransport.hpp"
#include "shared/protocol/Envelope.hpp"

namespace protocol {

/// Sends an ERROR envelope to one connection only, correlated to its requestId. Introduced to avoid LoginUseCase, AuthGuard, and MakeMoveUseCase each independently duplicating this same "build an error envelope, send it" pair.
inline void sendError(ITransport& transport, const std::string& connectionId, const std::string& requestId,
                       const std::string& code, const std::string& message) {
    transport.send(connectionId, errorEnvelope(requestId, code, message));
}

}  // namespace protocol
