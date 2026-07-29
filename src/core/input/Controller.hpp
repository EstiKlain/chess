#pragma once

#include <functional>
#include <optional>
#include <utility>

#include "model/Board.hpp"
#include "input/BoardMapper.hpp"
#include "engine/MoveRequest.hpp"
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
          requestJumpCallback_([this, &requester](int row, int col)
                                { requester.requestJump(row, col, localColor_); }) {}

    void handleClick(int x, int y);
    void requestMove(const MoveRequest &request);

    // Symmetric to handleClick, but for the jump command: pixel->cell
    // mapping only, no selection state machine (jump is a single-shot
    // request, unlike the two-click move flow).
    void handleJumpClick(int x, int y);

    bool hasSelection() const { return selection_.active; }
    int selectedRow() const { return selection_.row; }
    int selectedCol() const { return selection_.col; }

    // The local player's own color ('w'/'b'), used only as a client-side UX
    // pre-check (instant rejection of an opponent's piece without a round
    // trip to the server) - never the authoritative check, which always
    // lives server-side (GameEngine::requestMove/requestJump). '\0' (the
    // default) means "no local player known yet / unrestricted" - matches
    // MoveRequest::requesterColor's sentinel convention, and is what
    // ScriptRunner/tests get for free by never calling this setter.
    void setLocalColor(char color) { localColor_ = color; }

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
    char localColor_ = '\0';
    std::function<void(MoveRequest)> requestMoveCallback_;
    std::function<void(int, int)> requestJumpCallback_;
};