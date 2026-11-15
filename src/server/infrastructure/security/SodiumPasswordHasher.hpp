#pragma once

#include "server/domain_ports/IPasswordHasher.hpp"

// Real IPasswordHasher adapter, backed by libsodium's crypto_pwhash_str/
// crypto_pwhash_str_verify (Argon2id). Sibling to SodiumTokenGenerator, not
// merged into it - see that class's own header comment for why. Calling
// sodium_init() again here (on top of SodiumTokenGenerator's own call) is
// safe: libsodium documents it as idempotent and thread-safe.
class SodiumPasswordHasher : public IPasswordHasher {
public:
    SodiumPasswordHasher();

    std::string hash(const std::string& plainPassword) override;
    bool verify(const std::string& plainPassword, const std::string& hash) override;
};
