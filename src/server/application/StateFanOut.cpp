#include "server/application/StateFanOut.hpp"

#include <vector>

#include "server/protocol/Envelope.hpp"
#include "server/protocol/mappers/GameSnapshotMapper.hpp"

namespace StateFanOut {

void broadcast(GameSession& session, ConnectionManager& connections, IIdentityStore& identities,
                ITransport& transport, const std::string& originConnectionId, const std::string& originRequestId) {
    const auto recipients = connections.connectionsFor(&session);

    // Built once - identical for every recipient of this STATE_UPDATE, only
    // the top-level role/requestId vary per recipient below.
    std::vector<PlayerDto> players;
    players.reserve(recipients.size());
    for (const auto& [id, binding] : recipients) {
        players.push_back(PlayerDto{id, std::string(1, binding.color), identities.usernameFor(id).value_or("")});
    }

    const GameSnapshot snapshot = session.snapshot();
    for (const auto& [recipientId, binding] : recipients) {
        // Only the request's own originator gets its requestId echoed back -
        // everyone else never sent it, so giving them the same id would
        // wrongly imply they have a pending request of their own by that name.
        const std::string& recipientRequestId = (recipientId == originConnectionId) ? originRequestId : "";
        transport.send(recipientId,
                        protocol::envelope("STATE_UPDATE", recipientRequestId,
                                            GameSnapshotMapper::toJson(snapshot, binding.color, players)));
    }
}

}  // namespace StateFanOut
