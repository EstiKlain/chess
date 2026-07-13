#include "GameEngine.hpp"

#include <iostream>
#include <sstream>

#include "Board.hpp"
#include "BoardMapper.hpp"
#include "BoardParser.hpp"
#include "BoardPrinter.hpp"
#include "Controller.hpp"
#include "GameOverRule.hpp"
#include "MoveLegality.hpp"
#include "Movement.hpp"
#include "Moves.hpp"
#include "RuleEngine.hpp"
#include "config.hpp"

MoveResult GameEngine::requestMove(const MoveRequest &request)
{
    if (gameOver_)
        return {false, "game_over"};

    const int fromRow = request.from.row;
    const int fromCol = request.from.col;
    const int toRow = request.to.row;
    const int toCol = request.to.col;

    if (fromRow < 0 || fromCol < 0 || fromRow >= board_.rows() || fromCol >= board_.cols())
        return {false, "outside_board"};

    if (arbiter_.hasActiveMotion())
        return {false, "motion_in_progress"};

    const std::string selected = board_.grid[fromRow][fromCol];

    PieceMove m;
    m.fromRow = fromRow;
    m.fromCol = fromCol;
    m.toRow = toRow;
    m.toCol = toCol;
    m.startMs = elapsedMs_;
    m.piece = selected;

    const char piece = isEmpty(selected) ? '\0' : pieceOf(selected);
    const double speed = config::statsFor(piece).speedCellsPerSec;
    const double dist = cellDistance(fromRow, fromCol, toRow, toCol);
    m.durationMs = (speed > 0.0) ? (long)(dist / speed * 1000.0) : 0;

    const MoveLegality legality = isMoveLegal(board_, m, piece, rules_);
    if (!legality.isValid)
        return {false, legality.reason};

    arbiter_.startMotion(m);
    return {true, legality.reason};
}

JumpResult GameEngine::requestJump(int row, int col)
{
    if (gameOver_)
        return {false, "game_over"};

    if (row < 0 || col < 0 || row >= board_.rows() || col >= board_.cols())
        return {false, "outside_board"};

    const std::string &token = board_.grid[row][col];
    if (isEmpty(token))
        return {false, "empty_source"};

    if (arbiter_.isPieceInFlight(row, col))
        return {false, "motion_in_progress"};

    if (arbiter_.hasActiveJumpAt(row, col))
        return {false, "jump_in_progress"};

    JumpMove j;
    j.row = row;
    j.col = col;
    j.startMs = elapsedMs_;
    j.durationMs = config::JUMP_DURATION_MS;
    j.piece = token;
    arbiter_.startJump(j);
    return {true, "legal"};
}

void GameEngine::wait(long ms)
{
    if (gameOver_)
        return;

    elapsedMs_ += ms;
    std::vector<std::string> captured = arbiter_.resolveMoves(board_, elapsedMs_ , rules_);
    if (isGameOver(captured))
        gameOver_ = true;
}

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
