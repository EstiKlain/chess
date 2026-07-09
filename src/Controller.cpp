#include "Controller.hpp"

#include <functional>

#include "BoardMapper.hpp"
#include "Engine.hpp"

namespace
{
    void clearSelection(GameState &state)
    {
        state.selection = Selection{};
    }
}

void Controller::handleClick(int x, int y)
{
    if (x < 0 || y < 0)
    {
        if (state_.selection.active)
        {
            clearSelection(state_);
        }
        return;
    }

    const auto position = BoardMapper::pixelToCell(x, y, state_.board.rows(), state_.board.cols());
    if (!position.has_value())
    {
        if (state_.selection.active)
        {
            clearSelection(state_);
        }
        return;
    }

    const int row = position->row;
    const int col = position->col;
    const std::string &token = state_.board.grid[row][col];

    if (state_.selection.active)
    {
        const std::string &selectedToken = state_.board.grid[state_.selection.row][state_.selection.col];
        const bool sameColor = !isEmpty(token) && colorOf(token) == colorOf(selectedToken);

        if (sameColor)
        {
            state_.selection = {true, row, col, state_.elapsedMs};
        }
        else
        {
            requestMove({{state_.selection.row, state_.selection.col}, {row, col}});
            clearSelection(state_);
        }
        return;
    }

    if (!isEmpty(token))
    {
        state_.selection = {true, row, col, state_.elapsedMs};
    }
}

void Controller::requestMove(const MoveRequest &request)
{
    if (!state_.selection.active)
    {
        return;
    }

    if (requestMoveCallback_)
    {
        requestMoveCallback_(request);
    }

    if (state_.selection.active)
    {
        clearSelection(state_);
    }
}
