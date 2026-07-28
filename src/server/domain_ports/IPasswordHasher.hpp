#pragma once

#include <string>

// Password hashing/verification, kept separate from IUserRepository -
// comparing a plaintext password against a stored hash is crypto logic,
// not persistence.
class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;

    /// Hashes a plaintext password for storage. The returned string embeds its own salt/parameters and is safe to store directly.
    /// @param plainPassword The plaintext password to hash.
    /// @return An opaque hash string suitable for storage and later verification.
    virtual std::string hash(const std::string& plainPassword) = 0;

    /// Checks a plaintext password against a previously stored hash.
    /// @param plainPassword The plaintext password to check.
    /// @param hash A hash previously returned by hash().
    /// @return true if plainPassword matches hash, false for any mismatch or malformed hash - never throws on bad input.
    virtual bool verify(const std::string& plainPassword, const std::string& hash) = 0;
};
