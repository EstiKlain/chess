#pragma once

#include "server/domain_ports/ITokenGenerator.hpp"

// Real ITokenGenerator adapter, backed by libsodium's randombytes_buf
// (CSPRNG). libsodium was chosen (over e.g. Windows CNG) specifically
// because it is cross-platform - this project's confirmed eventual
// deployment target is a Docker container (see CLAUDE.md's "Future-Docker
// readiness" note), almost certainly Linux, where a Windows-only API would
// not exist at all. Iteration 6's password hashing (crypto_pwhash_str/
// crypto_pwhash_str_verify - Argon2id) reuses the same library via a
// sibling class, SodiumPasswordHasher - kept separate from this class
// (rather than merged into it) since token generation and password hashing
// are independently swappable concerns, matching this codebase's
// one-port-one-adapter pattern elsewhere.
class SodiumTokenGenerator : public ITokenGenerator {
public:
    SodiumTokenGenerator();

    std::string generate() override;
};
