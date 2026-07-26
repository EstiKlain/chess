#pragma once

#include <optional>
#include <string>

// Port: application code depends on THIS, never on InMemoryIdentityStore
// directly - lets LoginUseCase/AuthGuard be unit tested with a fake, and
// lets a future SQLite-backed adapter (Iteration 6) replace the in-memory
// one without touching application/ at all. Deliberately named
// IIdentityStore, not ISessionStore: GameSession already means "a running
// game" in this codebase, so reusing "session" for login identity would
// make the two concepts hard to tell apart by name alone.
class IIdentityStore {
public:
    virtual ~IIdentityStore() = default;

    /// Associates connectionId with username. Overwrites any prior association for this connectionId - a re-LOGIN on the same connection replaces, it does not stack.
    virtual void login(const std::string& connectionId, const std::string& username) = 0;

    /// True if connectionId currently has an associated username.
    virtual bool isLoggedIn(const std::string& connectionId) const = 0;

    /// Returns the username associated with connectionId, or nullopt if not logged in.
    virtual std::optional<std::string> usernameFor(const std::string& connectionId) const = 0;

    /// Removes any association for connectionId. Called on disconnect so a later connection reusing this id, or a stale lookup, never sees a ghost username.
    virtual void logout(const std::string& connectionId) = 0;
};
