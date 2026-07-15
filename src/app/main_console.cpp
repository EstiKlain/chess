#include <iostream>
#include <string>

#include "io/BoardParser.hpp"
#include "engine/GameEngine.hpp"
#include "rules/PieceRules.hpp"
#include "texttests/ScriptRunner.hpp"

int main()
{
    std::string input, line;
    while (std::getline(std::cin, line))
        input += line + '\n';

    Sections sections = parseSections(input);

    RawBoard raw = parseRawGrid(sections.boardLines);

    try
    {
        validateBoard(raw);
    }
    catch (const BoardError &e)
    {
        std::cout << "ERROR " << e.code() << '\n';
        return 0;
    }

    Board board = buildBoard(raw);

    GameEngine engine(board, pieceRules::PieceRulesRegistry());

    runCommands(sections.commandLines, engine);
    return 0;
}
