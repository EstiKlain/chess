#include "legacy/ScriptRunner.hpp"

#include <iostream>
#include <sstream>

// #include "input/BoardMapper.hpp"
#include "legacy/BoardParser.hpp"
#include "legacy/BoardPrinter.hpp"
#include "input/Controller.hpp"

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
            controller.handleJumpClick(x, y);
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