#include "server/infrastructure/security/SodiumPasswordHasher.hpp"

#include <sodium.h>

#include <array>
#include <stdexcept>

SodiumPasswordHasher::SodiumPasswordHasher() {
    if (sodium_init() < 0) {
        throw std::runtime_error("failed to initialize libsodium");
    }
}

std::string SodiumPasswordHasher::hash(const std::string& plainPassword) {
    std::array<char, crypto_pwhash_STRBYTES> out{};
    if (crypto_pwhash_str(out.data(), plainPassword.c_str(), plainPassword.size(),
                           crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("password hashing failed (out of memory)");
    }
    return std::string(out.data());
}

bool SodiumPasswordHasher::verify(const std::string& plainPassword, const std::string& hash) {
    if (hash.size() >= crypto_pwhash_STRBYTES) {
        return false;
    }
    return crypto_pwhash_str_verify(hash.c_str(), plainPassword.c_str(), plainPassword.size()) == 0;
}
