#include "server/infrastructure/security/SodiumTokenGenerator.hpp"

#include <sodium.h>

#include <array>
#include <cstdio>
#include <stdexcept>

namespace {
constexpr std::size_t kTokenBytes = 16;  // 128 bits

std::string toHex(const std::array<unsigned char, kTokenBytes>& bytes) {
    static const char* digits = "0123456789abcdef";
    std::string hex(bytes.size() * 2, '0');
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        hex[2 * i] = digits[(bytes[i] >> 4) & 0x0F];
        hex[2 * i + 1] = digits[bytes[i] & 0x0F];
    }
    return hex;
}
}  // namespace

SodiumTokenGenerator::SodiumTokenGenerator() {
    if (sodium_init() < 0) {
        throw std::runtime_error("failed to initialize libsodium");
    }
}

std::string SodiumTokenGenerator::generate() {
    std::array<unsigned char, kTokenBytes> bytes{};
    randombytes_buf(bytes.data(), bytes.size());
    return toHex(bytes);
}
