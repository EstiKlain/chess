#include "server/application/GameSessionStore.hpp"

GameSessionStore::GameSessionStore(std::function<GameEngine()> engineFactory)
    : engineFactory_(std::move(engineFactory)) {}

GameSession& GameSessionStore::create(bool ratingEligible) {
    sessions_.push_back(std::make_unique<GameSession>(engineFactory_(), ratingEligible));
    return *sessions_.back();
}

std::vector<GameSession*> GameSessionStore::allSessions() const {
    std::vector<GameSession*> result;
    result.reserve(sessions_.size());
    for (const auto& session : sessions_) {
        result.push_back(session.get());
    }
    return result;
}
