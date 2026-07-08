#include "Engine.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "Board.hpp"
#include "Movement.hpp"
#include "config.hpp"

void resolveMoves(GameState &st)
{
    std::vector<size_t> due;
    std::vector<PieceMove> stillMoving;
    for (size_t i = 0; i < st.activeMoves.size(); ++i)
    {
        const PieceMove &m = st.activeMoves[i];
        if (st.elapsedMs >= m.startMs + m.durationMs)
            due.push_back(i);
        else
            stillMoving.push_back(m);
    }

    std::sort(due.begin(), due.end(), [&](size_t a, size_t b)
              {
        long ta = st.activeMoves[a].startMs + st.activeMoves[a].durationMs;
        long tb = st.activeMoves[b].startMs + st.activeMoves[b].durationMs;
        if (ta != tb) return ta < tb;
        return a < b; });

    for (size_t idx : due)
    {
        const PieceMove &m = st.activeMoves[idx];
        std::string &target = st.board.grid[m.toRow][m.toCol];
        if (isEmpty(target) || colorOf(target) != m.piece[0])
        {
            target = m.piece;
        }
        else
        {
            std::string &origin = st.board.grid[m.fromRow][m.fromCol];
            if (isEmpty(origin))
                origin = m.piece;
        }
    }

    st.activeMoves = stillMoving;
}

void sendMove(GameState &st, int player, int toRow, int toCol)
{
    Selection &sel = st.selections[player];
    const std::string selected = st.board.grid[sel.row][sel.col];

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

    if (isLegalMove(st.board, m, piece))
    {
        st.board.grid[m.fromRow][m.fromCol] = ".";
        st.activeMoves.push_back(m);
    }
    sel = Selection{};
}

void handleClick(GameState &st, int player, int x, int y)
{
    if (player < 0 || player >= (int)st.selections.size())
        return;

    if (x < 0 || y < 0)
        return;

    int col = x / config::CELL_SIZE;
    int row = y / config::CELL_SIZE;

    if (row < 0 || row >= st.board.rows() ||
        col < 0 || col >= st.board.cols())
        return;

    const std::string &token = st.board.grid[row][col];
    bool ownPiece = !isEmpty(token) && playerIndexOf(colorOf(token)) == player;

    if (st.selections[player].active)
    {
        if (ownPiece)
        {
            st.selections[player] = {true, row, col, st.elapsedMs}; // reselect
        }
        else
        {
            sendMove(st, player, row, col); // complete: move or capture
        }
        return;
    }

    if (ownPiece)
    {
        st.selections[player] = {true, row, col, st.elapsedMs}; // open a fresh selection
    }
}

void handleWait(GameState &st, long ms)
{
    st.elapsedMs += ms;
    resolveMoves(st);
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
            int player, x, y;
            ss >> player >> x >> y;
            handleClick(st, player, x, y);
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
