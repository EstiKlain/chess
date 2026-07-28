#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "server/domain_ports/IUserRepository.hpp"

// Temporary IUserRepository stand-in, wired into main_server.cpp until
// PostgresUserRepository is registered in the build (see CMakeLists.txt
// comments and docs/kungfu_chess_server_plan (1).md's Iteration 6 section).
// Unlike InMemoryIdentityStore, this class is reachable from two threads -
// LoginUseCase on the io_service thread, EloUpdateUseCase on the tick
// thread (see IUserRepository.hpp's own callers) - so every public method
// takes a lock, the same shape as GameSession's mutex_.
class InMemoryUserRepository : public IUserRepository {
public:
    std::optional<UserRecord> findByUsername(const std::string& username) override;
    void createUser(const std::string& username, const std::string& passwordHash) override;
    void updateElo(const std::string& username, int newElo) override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, UserRecord> users_;
};
