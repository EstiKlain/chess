#include "server/application/DisconnectUseCase.hpp"

#include <cmath>
#include <unordered_set>

#include "server/application/GameSession.hpp"
#include "server/protocol/Envelope.hpp"
#include "server/protocol/dto/DisconnectDto.hpp"
#include "shared/protocol/config.hpp"

DisconnectUseCase::DisconnectUseCase(PlayerSessionRegistry& sessions, ConnectionManager& connections,
                                      ITransport& transport, IClock& clock)
    : sessions_(sessions), connections_(connections), transport_(transport), clock_(clock) {}

void DisconnectUseCase::onDisconnected(const std::string& connectionId) {
    sessions_.markDisconnected(connectionId, clock_.nowMs());
}

void DisconnectUseCase::tick(long nowMs) {
    const auto disconnected = sessions_.disconnectedEntries();

    std::unordered_set<std::string> stillDisconnected;
    for (const auto& [token, entry] : disconnected) {
        stillDisconnected.insert(token);
    }
    for (auto it = lastSentSecondsLeft_.begin(); it != lastSentSecondsLeft_.end();) {
        if (stillDisconnected.count(it->first) == 0) {
            it = lastSentSecondsLeft_.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& [token, entry] : disconnected) {
        const long elapsed = nowMs - entry.disconnectedAtMs.value_or(nowMs);

        if (elapsed >= server_config::kReconnectWindowMs) {
            if (entry.session != nullptr) {
                entry.session->resign(entry.color);
            }
            sessions_.remove(token);
            lastSentSecondsLeft_.erase(token);
            continue;
        }

        const int secondsLeft =
            static_cast<int>(std::ceil((server_config::kReconnectWindowMs - elapsed) / 1000.0));
        auto sentIt = lastSentSecondsLeft_.find(token);
        if (sentIt != lastSentSecondsLeft_.end() && sentIt->second == secondsLeft) {
            continue;
        }
        lastSentSecondsLeft_[token] = secondsLeft;

        for (const auto& [recipientId, binding] : connections_.connectionsFor(entry.session)) {
            if (binding.color != entry.color) {
                transport_.send(recipientId,
                                 protocol::envelope("DISCONNECT_COUNTDOWN", nlohmann::json(DisconnectDto{secondsLeft})));
            }
        }
    }
}
