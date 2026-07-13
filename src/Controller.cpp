#include "Controller.hpp"

std::optional<Position> Controller::mapToCell(int x, int y) const
{
    return BoardMapper::pixelToCell(x, y, board_.rows(), board_.cols());
}

void Controller::handleClick(int x, int y)
{
    const auto position = mapToCell(x, y);
    if (!position.has_value())
    {
        if (selection_.active)
        {
            selection_ = Selection{};
        }
        return;
    }

    const int row = position->row;
    const int col = position->col;
    const std::string &token = board_.grid[row][col];

    if (selection_.active)
    {
        const std::string &selectedToken = board_.grid[selection_.row][selection_.col];
        const bool sameColor = !isEmpty(token) && colorOf(token) == colorOf(selectedToken);

        if (sameColor)
        {
            selection_ = {true, row, col};
        }
        else
        {
            requestMove({{selection_.row, selection_.col}, {row, col}});
            selection_ = Selection{};
        }
        return;
    }

    if (!isEmpty(token))
    {
        selection_ = {true, row, col};
    }
}

void Controller::handleJumpClick(int x, int y)
{
    const auto position = mapToCell(x, y);
    if (!position.has_value())
        return;

    if (requestJumpCallback_)
        requestJumpCallback_(position->row, position->col);
}

void Controller::requestMove(const MoveRequest &request)
{
    if (!selection_.active)
    {
        return;
    }

    if (requestMoveCallback_)
    {
        requestMoveCallback_(request);
    }

    if (selection_.active)
    {
        selection_ = Selection{};
    }
}