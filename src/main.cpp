#include <iostream>
#include <string>

#include "Board.hpp"
#include "Engine.hpp"
#include "GameState.hpp"
#include "config.hpp"

int main() {
    std::string input, line;
    while (std::getline(std::cin, line)) input += line + '\n';

    Sections sections = parseSections(input);

    GameState state;
    state.board = parseBoard(sections.boardLines);
    state.selections.assign(config::NUM_PLAYERS, Selection{});

    try {
        validateBoard(state.board);
    } catch (const BoardError& e) {
        std::cout << "ERROR " << e.code() << '\n';
        return 0;
    }

    runCommands(sections.commandLines, state);
    return 0;
}
