#include <iostream>
#include <string>

#include "BoardParser.hpp"
#include "GameEngine.hpp"
#include "config.hpp"

int main() {
    std::string input, line;
    while (std::getline(std::cin, line)) input += line + '\n';

    Sections sections = parseSections(input);

    Board board = parseBoard(sections.boardLines);

    try {
        validateBoard(board);
    } catch (const BoardError& e) {
        std::cout << "ERROR " << e.code() << '\n';
        return 0;
    }

    GameEngine engine(board);
    runCommands(sections.commandLines, engine);
    return 0;
}
