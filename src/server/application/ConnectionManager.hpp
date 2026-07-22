#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class GameSession;

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
};

class ConnectionManager {
public:
    /// Assigns a connection to a session and a color: first connection gets 'w', second gets 'b', every connection after that is rejected (server only supports 2 players per session today).
    ConnectionOutcome onConnected(const std::string& connectionId, GameSession* session);

    /// Removes a connection's binding, freeing its color for a future connection.
    void onDisconnected(const std::string& connectionId);

    /// Looks up which session/color a connection is bound to, if any.
    std::optional<ConnectionBinding> sessionFor(const std::string& connectionId) const;

    /// Lists every (connectionId, binding) pair currently bound to the given session (used to fan a STATE_UPDATE out to both players of one game). Returns bindings directly, not just ids, so callers don't need a second sessionFor() lookup per recipient just to read the color back.
    std::vector<std::pair<std::string, ConnectionBinding>> connectionsFor(const GameSession* session) const;

    /// True if this connection is currently registered.
    bool isConnected(const std::string& connectionId) const;

    /// Total number of currently registered connections.
    std::size_t connectionCount() const;

private:
    std::unordered_map<std::string, ConnectionBinding> connections_;
};
