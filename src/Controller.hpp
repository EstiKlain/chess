#pragma once

#include <functional>
#include <utility>

#include "Board.hpp"
#include "MoveRequest.hpp"

class Controller
{
public:
    Controller(Board &board, std::function<void(MoveRequest)> requestMoveCallback)
        : board_(board), requestMoveCallback_(std::move(requestMoveCallback)) {}

    template <typename MoveRequester>
    explicit Controller(Board &board, MoveRequester &requester)
        : board_(board), requestMoveCallback_([&requester](MoveRequest request)
                                              { requester.requestMove(request); }) {}

    void handleClick(int x, int y);
    void requestMove(const MoveRequest &request);

    // Read-only query needed now that Selection is private to Controller
    // (Stage 2, item 1). Existing tests used to check GameState::selection
    // directly; since that field no longer exists anywhere outside
    // Controller, this is the minimal replacement so those tests can still
    // observe "is something selected" without exposing the Selection type
    // itself. Flagged separately in chat - not part of the original plan.
    bool hasSelection() const { return selection_.active; }
    int selectedRow() const { return selection_.row; }
    int selectedCol() const { return selection_.col; }

private:
    struct Selection
    {
        bool active = false;
        int row = 0, col = 0;
    };

    Board &board_;
    Selection selection_;
    std::function<void(MoveRequest)> requestMoveCallback_;
};
