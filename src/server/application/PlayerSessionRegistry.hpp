#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "server/domain_ports/ITokenGenerator.hpp"

class GameSession;

// What a sessionToken currently identifies: which game and color it plays,
// and either a live connectionId (connected) or a disconnectedAtMs
// timestamp (mid reconnect-window) - never both.
struct PlayerSessionEntry {
    std::string username;
    char color = ' ';
    GameSession* session = nullptr;
    std::optional<std::string> connectionId;
    std::optional<long> disconnectedAtMs;
};

// Durable "who is entitled to reconnect" registry, keyed by sessionToken -
// deliberately separate from ConnectionManager, which stays the ephemeral
// "who is connected right now" map (connectionId -> binding, erased on
// disconnect). PlayerSessionRegistry has no dependency on ConnectionManager
// at all - the dependency direction is one-way (ConnectionManager consults
// this registry via onConnected's seat-reservation check), never the
// reverse, avoiding a circular include between the two.
class PlayerSessionRegistry {
public:
    explicit PlayerSessionRegistry(ITokenGenerator& tokenGenerator);

    /// Generates a fresh token and registers a brand-new, connected entry. Called by LoginUseCase on a successful LOGIN.
    std::string registerNew(const std::string& username, char color, GameSession* session,
                             const std::string& connectionId);

    /// Looks up an entry by token.
    std::optional<PlayerSessionEntry> find(const std::string& token) const;

    /// Marks the entry currently bound to connectionId as disconnected at nowMs (clears connectionId, records disconnectedAtMs). No-op if connectionId has no entry.
    void markDisconnected(const std::string& connectionId, long nowMs);

    /// Re-binds token to newConnectionId, clearing disconnectedAtMs - but only if the entry is currently disconnected (connectionId == nullopt). Fails (returns false) for an unknown token OR a token that is already connected (e.g. a second reconnect attempt racing the first) - both cases mean "no game left to rejoin this way" to the caller.
    bool reconnect(const std::string& token, const std::string& newConnectionId);

    /// Removes the entry entirely - called once DisconnectUseCase resigns it; the seat is freed for good, not just "still disconnected".
    void remove(const std::string& token);

    /// True if `color` is held by an entry for `session` that still exists in the registry at all (connected, or disconnected-but-still-within-its-window) - used by ConnectionManager::onConnected's seat-reservation check.
    bool colorReserved(const GameSession* session, char color) const;

    /// True if any entry is currently disconnected (used by DisconnectUseCase::tick to know which tokens to check/prune).
    std::vector<std::pair<std::string, PlayerSessionEntry>> disconnectedEntries() const;

private:
    ITokenGenerator& tokenGenerator_;
    std::unordered_map<std::string, PlayerSessionEntry> byToken_;
};
