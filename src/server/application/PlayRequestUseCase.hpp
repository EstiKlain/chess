#pragma once

#include <functional>
#include <string>

#include <nlohmann/json.hpp>

#include "server/application/ConnectionManager.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/domain_ports/IClock.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/IMatchmakingQueue.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/domain_ports/IUserRepository.hpp"

// Owns the matchmaking queue's admission and exit rules. Mirrors
// DisconnectUseCase's shape: one event-triggered entry point (handlePlay)
// plus one tick-driven entry point (tick) for the same queue.
class PlayRequestUseCase {
public:
    /// Called once two players are matched. Injected rather than calling MatchStarter::start directly, so this class never needs to include GameSessionStore.hpp.
    using MatchStartHandler = std::function<void(const WaitingPlayer& waitingPlayer, const WaitingPlayer& joiningPlayer)>;

    PlayRequestUseCase(IMatchmakingQueue& queue, ConnectionManager& connections, PlayerSessionRegistry& sessions,
                        IIdentityStore& identities, IUserRepository& users, ITransport& transport, IClock& clock,
                        MatchStartHandler onMatch);

    /// Matches connectionId immediately if a same-Elo-range opponent is already waiting, otherwise enqueues it.
    void handlePlay(const std::string& connectionId, const std::string& requestId, const nlohmann::json& payload);

    /// Sends NO_MATCH_FOUND to anyone who has waited past kMatchmakingTimeoutMs.
    void tick(long nowMs);

    /// Removes connectionId from the queue, if it was waiting there.
    void onDisconnected(const std::string& connectionId);

private:
    IMatchmakingQueue& queue_;
    ConnectionManager& connections_;
    PlayerSessionRegistry& sessions_;
    IIdentityStore& identities_;
    IUserRepository& users_;
    ITransport& transport_;
    IClock& clock_;
    MatchStartHandler onMatch_;
};
