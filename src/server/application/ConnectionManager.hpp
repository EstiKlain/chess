#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "server/config.hpp"

class GameSession;

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
    /// Binds a connectionId to a specific session+color - used by MatchStarter (a fresh match) and ReconnectUseCase (a reclaimed seat), both of which already know the exact color to assign.
    void bindKnown(const std::string& connectionId, GameSession* session, char color);

    /// Removes a connection's binding, freeing its color for a future connection.
    void onDisconnected(const std::string& connectionId);

    /// Looks up which session/color a connection is bound to, if any.
    std::optional<ConnectionBinding> sessionFor(const std::string& connectionId) const;

    /// Updates the live Elo cache for an already-bound connection. No-op if connectionId isn't bound (e.g. it disconnected in between). Called by MatchStarter (initial value), ReconnectUseCase (refresh on reconnect), and EloUpdateUseCase (after a game ends).
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
