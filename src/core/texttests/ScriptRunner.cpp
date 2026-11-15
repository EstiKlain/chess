#include "texttests/ScriptRunner.hpp"

#include <iostream>
#include <sstream>

// #include "input/BoardMapper.hpp"
#include "engine/SnapshotSync.hpp"
#include "io/BoardParser.hpp"
#include "io/BoardPrinter.hpp"
#include "input/Controller.hpp"

void runCommands(const std::vector<std::string> &commands, GameEngine &engine)
{
    // Mirror, not a live reference into GameEngine - same pattern main_gui.cpp
    // uses for the networked client. Controller only ever reads it
    // (pieceAt/rows/cols), so a mirror re-synced from snapshot() before each
    // "print board" is a faithful stand-in.
    Board mirrorBoard;
    syncBoardFromSnapshot(mirrorBoard, engine.snapshot());
    Controller controller(mirrorBoard, engine);

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
            syncBoardFromSnapshot(mirrorBoard, engine.snapshot());
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
                std::cout << formatBoard(mirrorBoard);
        }
    }
}