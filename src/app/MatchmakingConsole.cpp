#include "app/MatchmakingConsole.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {
constexpr int kMatchmakingPollIntervalMs = 300;
}

bool waitForMatchInteractively(ServerConnection& connection) {
    connection.requestPlay();
    while (true) {
        while (connection.matchmakingState() == MatchmakingState::Searching) {
            std::cout << "\rSearching for opponent..." << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(kMatchmakingPollIntervalMs));
        }
        if (connection.matchmakingState() == MatchmakingState::Matched) {
            std::cout << "\rMatch found! Opponent: " << connection.matchedOpponentName().value_or("?")
                       << "                         \n";
            return true;
        }

        if (connection.matchmakingState() == MatchmakingState::AlreadyInGame) {
            std::cout << "\rThis account already has an active game running elsewhere. "
                          "Close this window and return to it.                         \n";
            return false;
        }

        std::cout << "\rNo opponent found within 60s. Search again? [Y/n] ";
        std::string answer;
        std::getline(std::cin, answer);
        if (!answer.empty() && (answer[0] == 'n' || answer[0] == 'N')) {
            return false;
        }
        connection.requestPlay();
    }
}
