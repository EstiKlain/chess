#pragma once

#include <mutex>
#include <optional>
#include <string>

#include <pqxx/pqxx>

#include "server/domain_ports/IUserRepository.hpp"

// NOT REGISTERED IN CMakeLists.txt YET - see docs/kungfu_chess_server_plan
// (1).md's Iteration 6 section. This file is source-only until the Docker
// phase adds find_package(PostgreSQL)/FetchContent(libpqxx) and links it
// into chess_server; it has never been compiled against a real libpqxx and
// should be re-checked against IUserRepository's then-current shape before
// being wired in. Written now anyway (a deliberate, recorded decision, not
// an oversight) so the adapter is fully designed and ready when Docker
// lands.
//
// Schema this class assumes already exists:
//   CREATE TABLE users (
//       id SERIAL PRIMARY KEY,
//       username TEXT UNIQUE NOT NULL,
//       password_hash TEXT NOT NULL,
//       elo INTEGER NOT NULL DEFAULT 1200,  -- matches server_config::kInitialElo
//       created_at TIMESTAMPTZ NOT NULL DEFAULT now()
//   );
//
// Constructor takes an already-open connection, never a connection string -
// the composition root (main_server.cpp) is the only place that ever reads
// the actual connection string (e.g. from the CHESS_DB_URL environment
// variable), matching FileLogger's std::ostream& precedent. Owns its own
// mutex around each transaction for the same cross-thread reason as
// InMemoryUserRepository (pqxx::connection is not safe for concurrent use
// from multiple threads, and this class is called from both the
// io_service thread via LoginUseCase and the tick thread via
// EloUpdateUseCase).
class PostgresUserRepository : public IUserRepository {
public:
    explicit PostgresUserRepository(pqxx::connection& connection);

    std::optional<UserRecord> findByUsername(const std::string& username) override;
    void createUser(const std::string& username, const std::string& passwordHash) override;
    void updateElo(const std::string& username, int newElo) override;

private:
    std::mutex mutex_;
    pqxx::connection& connection_;
};
