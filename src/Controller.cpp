#include "Controller.hpp"

std::optional<Position> Controller::mapToCell(int x, int y) const
{
    return BoardMapper::pixelToCell(x, y, board_.rows(), board_.cols());
}

void Controller::clearSelection()
{
    selection_.active = false;
    selection_.row = 0;
    selection_.col = 0;
}

void Controller::setSelection(int row, int col)
{
    selection_.active = true;
    selection_.row = row;
    selection_.col = col;
}

void Controller::handleClick(int x, int y)
{
    const auto position = mapToCell(x, y);
    if (!position.has_value())
    {
        if (selection_.active)
        {
            clearSelection();
        }
        return;
    }

    const int row = position->row;
    const int col = position->col;
    const Piece *clicked = board_.pieceAt(Position{row, col});

    if (selection_.active)
    {
        const Piece *selectedPiece = board_.pieceAt(Position{selection_.row, selection_.col});
        const bool sameColor = clicked != nullptr && selectedPiece != nullptr && clicked->color == selectedPiece->color;

        if (sameColor)
        {
            selection_ = {true, row, col};
        }
        else
        {
            requestMove({{selection_.row, selection_.col}, {row, col}});
            clearSelection();
        }
        return;
    }

    if (clicked != nullptr)
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
        clearSelection();
    }
}
