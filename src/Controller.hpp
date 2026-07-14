#pragma once

#include <functional>
#include <optional>
#include <utility>

#include "model/Board.hpp"
#include "BoardMapper.hpp"
#include "MoveRequest.hpp"
#include "model/Position.hpp"

class Controller
{
public:
    Controller(Board &board, std::function<void(MoveRequest)> requestMoveCallback)
        : board_(board), requestMoveCallback_(std::move(requestMoveCallback)) {}

    template <typename MoveRequester>
    explicit Controller(Board &board, MoveRequester &requester)
        : board_(board),
          requestMoveCallback_([&requester](MoveRequest request)
                                { requester.requestMove(request); }),
          requestJumpCallback_([&requester](int row, int col)
                                { requester.requestJump(row, col); }) {}

    void handleClick(int x, int y);
    void requestMove(const MoveRequest &request);

    // Symmetric to handleClick, but for the jump command: pixel->cell
    // mapping only, no selection state machine (jump is a single-shot
    // request, unlike the two-click move flow).
    void handleJumpClick(int x, int y);

    bool hasSelection() const { return selection_.active; }
    int selectedRow() const { return selection_.row; }
    int selectedCol() const { return selection_.col; }

private:
    struct Selection
    {
        bool active = false;
        int row = 0, col = 0;
    };

    // Shared pixel->cell translation used by both handleClick and
    // handleJumpClick, so the mapping logic itself is written once.
    std::optional<Position> mapToCell(int x, int y) const;

    void clearSelection();
    void setSelection(int row, int col);
    Board &board_;
    Selection selection_;
    std::function<void(MoveRequest)> requestMoveCallback_;
    std::function<void(int, int)> requestJumpCallback_;
};