#pragma once

#include <optional>
#include <string>

// A persisted user account: username, password hash (never the plaintext
// password), and current Elo rating. The one row this port reads/writes -
// findByUsername/createUser/updateElo all act on the same entity, which is
// why they live on one port rather than three (unlike ITokenGenerator vs.
// IPasswordHasher, which are independently swappable technologies).
struct UserRecord {
    std::string username;
    std::string passwordHash;
    int elo;
};

// Durable user-account storage: registration, credential lookup, and Elo
// persistence. Deliberately has no verifyPassword method - comparing a
// plaintext password against a hash is crypto logic (IPasswordHasher's
// job), not persistence; this port stays pure CRUD.
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    /// Looks up a user by username. Returns std::nullopt if no such account exists - the signal LoginUseCase uses to decide register vs. verify.
    /// @param username The account's username.
    /// @return The stored record, or std::nullopt if the username is unknown.
    virtual std::optional<UserRecord> findByUsername(const std::string& username) = 0;

    /// Creates a brand-new account with an already-hashed password. Caller must have already confirmed findByUsername returned std::nullopt for this username.
    /// @param username The new account's username.
    /// @param passwordHash The already-hashed password (never a plaintext password).
    virtual void createUser(const std::string& username, const std::string& passwordHash) = 0;

    /// Overwrites the stored Elo rating for an existing user.
    /// @param username The account to update.
    /// @param newElo The new Elo rating to store.
    virtual void updateElo(const std::string& username, int newElo) = 0;
};
