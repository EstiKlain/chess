#include "server/infrastructure/persistence/PostgresUserRepository.hpp"

// NOT COMPILED YET - see the header's comment and
// docs/kungfu_chess_server_plan (1).md's Iteration 6 section. This is the
// intended implementation, to be sanity-checked against a real libpqxx
// once this file is registered in CMakeLists.txt at the start of the
// Docker phase.

PostgresUserRepository::PostgresUserRepository(pqxx::connection& connection) : connection_(connection) {}

std::optional<UserRecord> PostgresUserRepository::findByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(connection_);
    const pqxx::result rows =
        txn.exec_params("SELECT username, password_hash, elo FROM users WHERE username = $1", username);
    txn.commit();
    if (rows.empty()) {
        return std::nullopt;
    }
    const pqxx::row row = rows[0];
    return UserRecord{row["username"].as<std::string>(), row["password_hash"].as<std::string>(),
                       row["elo"].as<int>()};
}

void PostgresUserRepository::createUser(const std::string& username, const std::string& passwordHash) {
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(connection_);
    txn.exec_params("INSERT INTO users (username, password_hash) VALUES ($1, $2)", username, passwordHash);
    txn.commit();
}

void PostgresUserRepository::updateElo(const std::string& username, int newElo) {
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(connection_);
    txn.exec_params("UPDATE users SET elo = $1 WHERE username = $2", newElo, username);
    txn.commit();
}
