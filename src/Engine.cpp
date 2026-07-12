#include "Engine.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "Board.hpp"
#include "Controller.hpp"
#include "Movement.hpp"
#include "GameOverRule.hpp"
#include "RealTimeArbiter.hpp"
#include "RuleEngine.hpp"
#include "config.hpp"

void sendMove(GameState &st, const MoveRequest &request)
{
    if (st.gameOver || !st.selection.active) return;

    int fromRow = request.from.row;
    int fromCol = request.from.col;
    int toRow = request.to.row;
    int toCol = request.to.col;

    Selection &sel = st.selection;
    if (sel.row < 0 || sel.col < 0 || sel.row >= st.board.rows() || sel.col >= st.board.cols())
    {
        sel = Selection{};
        return;
    }

    if (toRow < 0 || toCol < 0 || toRow >= st.board.rows() || toCol >= st.board.cols())
    {
        sel = Selection{};
        return;
    }

    const std::string selected = st.board.grid[sel.row][sel.col];

    if (hasActiveMotion(st))
    {
        sel = Selection{};
        return;
    }

    PieceMove m;
    m.fromRow = sel.row;
    m.fromCol = sel.col;
    m.toRow = toRow;
    m.toCol = toCol;
    m.startMs = st.elapsedMs;
    m.piece = selected;

    char piece = pieceOf(selected);
    double speed = config::statsFor(piece).speedCellsPerSec;
    double dist = cellDistance(m.fromRow, m.fromCol, toRow, toCol);
    m.durationMs = (speed > 0.0) ? (long)(dist / speed * 1000.0) : 0;

    if (isMoveLegal(st.board, m, piece))
    {
        st.board.grid[m.fromRow][m.fromCol] = ".";
        st.activeMoves.push_back(m);
    }
    sel = Selection{};
}

void handleClick(GameState &st, int x, int y)
{
    Controller controller(st, [&](MoveRequest request)
                          { sendMove(st, request); });
    controller.handleClick(x, y);
}
void handleWait(GameState &st, long ms)
{
    if (st.gameOver)
    {
        return;
    }

    st.elapsedMs += ms;
    std::vector<std::string> captured = resolveMoves(st);
    if (isGameOver(captured))
    {
        st.gameOver = true;
    }
}

void runCommands(const std::vector<std::string> &commands, GameState &st)
{
    for (const auto &command : commands)
    {
        std::istringstream ss(command);
        std::string verb;
        ss >> verb;

        if (verb == "click")
        {
            int x, y;
            ss >> x >> y;
            handleClick(st, x, y);
        }
        else if (verb == "wait")
        {
            long ms;
            ss >> ms;
            handleWait(st, ms);
        }
        else if (verb == "print")
        {
            std::string rest;
            std::getline(ss, rest);
            if (trim(rest) == "board")
                std::cout << formatBoard(st.board);
        }
    }
}
