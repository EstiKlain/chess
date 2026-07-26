#include "server/application/GameSession.hpp"

MoveResult GameSession::requestMove(const MoveRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.requestMove(request);
}

JumpResult GameSession::requestJump(int row, int col) {
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.requestJump(row, col);
}

GameSnapshot GameSession::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.snapshot();
}

void GameSession::wait(long ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.wait(ms);
}
