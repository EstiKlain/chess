#include "ScriptRunner.hpp"

#include <iostream>
#include <sstream>

#include "BoardMapper.hpp"
#include "BoardParser.hpp"
#include "BoardPrinter.hpp"
#include "Controller.hpp"

void runCommands(const std::vector<std::string> &commands, GameEngine &engine)
{
    Controller controller(engine.board(), engine);

    for (const auto &command : commands)
    {
        std::istringstream ss(command);
        std::string verb;
        ss >> verb;

        if (verb == "click")
        {
            int x, y;
            ss >> x >> y;
            controller.handleClick(x, y);
        }
        else if (verb == "wait")
        {
            long ms;
            ss >> ms;
            engine.wait(ms);
        }
        else if (verb == "jump")
        {
            int x, y;
            ss >> x >> y;
            const auto position = BoardMapper::pixelToCell(x, y, engine.board().rows(), engine.board().cols());
            if (position.has_value())
                engine.requestJump(position->row, position->col);
        }
        else if (verb == "print")
        {
            std::string rest;
            std::getline(ss, rest);
            if (trim(rest) == "board")
                std::cout << formatBoard(engine.board());
        }
    }
}