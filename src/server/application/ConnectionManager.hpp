#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "server/config.hpp"

class GameSession;
class PlayerSessionRegistry;

// Result of ConnectionManager::onConnected: whether this connection got a
// seat at the table, and which color it was assigned if so.
struct ConnectionOutcome {
    bool accepted = false;
    char color = ' ';
};

// What a connection is bound to once accepted: which GameSession it plays
// in, and which color it plays. Kept as a connectionId -> binding map from
// map insertion, not a structural rework - see the plan's decisions log.
struct ConnectionBinding {
    GameSession* session = nullptr;
    char color = ' ';
    // Live Elo cache for this connection, refreshed only on login/reconnect/
    // game-end (never read from IUserRepository on the tick's hot path) -
    // see ConnectionManager::setElo and EloUpdateUseCase.
    int elo = server_config::kInitialElo;
};

class ConnectionManager {
public:
    /// Assigns a connection to a session and a color: first connection gets 'w', second gets 'b', every connection after that is rejected (server only supports 2 players per session today). A color already reserved in `sessions` (a disconnected player still within their reconnect window) is treated as taken too - it must not be handed to a new, unrelated connection.
    ConnectionOutcome onConnected(const std::string& connectionId, GameSession* session,
                                  const PlayerSessionRegistry& sessions);

    /// Directly binds a connectionId to a specific session+color, bypassing the w-then-b-then-reject assignment logic above - used only by ReconnectUseCase, which already knows the exact seat/color a reconnecting player owns (PlayerSessionRegistry::reconnect already enforced that only one connection can claim it).
    void bindKnown(const std::string& connectionId, GameSession* session, char color);

    /// Removes a connection's binding, freeing its color for a future connection.
    void onDisconnected(const std::string& connectionId);

    /// Looks up which session/color a connection is bound to, if any.
    std::optional<ConnectionBinding> sessionFor(const std::string& connectionId) const;

    /// Updates the live Elo cache for an already-bound connection. No-op if connectionId isn't bound (e.g. it disconnected in between). Called by LoginUseCase (initial value), ReconnectUseCase (refresh on reconnect), and EloUpdateUseCase (after a game ends).
    void setElo(const std::string& connectionId, int elo);

    /// Lists every (connectionId, binding) pair currently bound to the given session (used to fan a STATE_UPDATE out to both players of one game). Returns bindings directly, not just ids, so callers don't need a second sessionFor() lookup per recipient just to read the color back.
    std::vector<std::pair<std::string, ConnectionBinding>> connectionsFor(const GameSession* session) const;

    /// True if this connection is currently registered.
    bool isConnected(const std::string& connectionId) const;

    /// Total number of currently registered connections.
    std::size_t connectionCount() const;

private:
    std::unordered_map<std::string, ConnectionBinding> connections_;

    // Secondary index kept in sync by every mutator below, so connectionsFor()
    // is O(this session's players) instead of scanning every connection.
    std::unordered_map<const GameSession*, std::vector<std::string>> bySession_;
};
