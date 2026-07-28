#include "server/application/ReconnectUseCase.hpp"

#include <exception>
#include <vector>

#include "server/application/GameSession.hpp"
#include "server/application/StateFanOut.hpp"
#include "server/protocol/Envelope.hpp"
#include "server/protocol/dto/DisconnectDto.hpp"
#include "server/protocol/dto/ReconnectDto.hpp"

ReconnectUseCase::ReconnectUseCase(PlayerSessionRegistry& sessions, ConnectionManager& connections,
                                    IIdentityStore& identities, ITransport& transport)
    : sessions_(sessions), connections_(connections), identities_(identities), transport_(transport) {}

void ReconnectUseCase::handleReconnect(const std::string& connectionId, const std::string& requestId,
                                       const nlohmann::json& payload) {
    ReconnectDto dto;
    try {
        dto = payload.get<ReconnectDto>();
    } catch (const std::exception& ex) {
        protocol::sendError(transport_, connectionId, requestId, "MALFORMED_PAYLOAD",
                             std::string("malformed RECONNECT payload: ") + ex.what());
        return;
    }

    const auto entry = sessions_.find(dto.sessionToken);
    if (!entry.has_value() || !sessions_.reconnect(dto.sessionToken, connectionId)) {
        protocol::sendError(transport_, connectionId, requestId, "SESSION_EXPIRED",
                             "no game to reconnect to for this token");
        return;
    }

    connections_.bindKnown(connectionId, entry->session, entry->color);
    identities_.login(connectionId, entry->username);

    StateFanOut::broadcast(*entry->session, connections_, identities_, transport_, connectionId, requestId);

    // Tell the opponent's client the countdown it may be displaying is over
    // - STATE_UPDATE alone never carried that signal (it flows unconditionally
    // regardless of disconnect state). If the opponent is itself disconnected
    // right now, recipients simply won't include them - not an error, the
    // same "nobody there to tell" case DisconnectUseCase already accepts.
    const auto recipients = connections_.connectionsFor(entry->session);
    for (const auto& [id, binding] : recipients) {
        if (binding.color != entry->color) {
            transport_.send(id, protocol::envelope("DISCONNECT_COUNTDOWN", nlohmann::json(DisconnectDto{0})));
        }
    }
}
