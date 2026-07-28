#pragma once

#include <string>

// Port: application code depends on THIS, never on libsodium (or whatever
// crypto library is in use) directly - the only file in the entire
// codebase that #includes <sodium.h> is the concrete adapter
// (infrastructure/security/SodiumTokenGenerator). If the underlying library
// ever needs to change, only that one adapter changes.
class ITokenGenerator {
public:
    virtual ~ITokenGenerator() = default;

    /// Generates a fresh, unguessable token (hex-encoded, >=128 bits of
    /// entropy in the real implementation) - used as PlayerSessionRegistry's
    /// sessionToken, a bearer credential that reclaims a disconnected
    /// player's seat.
    virtual std::string generate() = 0;
};
