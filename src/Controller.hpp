#pragma once

#include <functional>
#include <utility>

#include "GameState.hpp"
#include "MoveRequest.hpp"

class Controller
{
public:
    Controller(GameState &state, std::function<void(MoveRequest)> requestMoveCallback)
        : state_(state), requestMoveCallback_(std::move(requestMoveCallback)) {}

    template <typename MoveRequester>
    explicit Controller(GameState &state, MoveRequester &requester)
        : state_(state), requestMoveCallback_([&requester](MoveRequest request)
                                              { requester.requestMove(request); }) {}

    void handleClick(int x, int y);
    void requestMove(const MoveRequest &request);

private:
    GameState &state_;
    std::function<void(MoveRequest)> requestMoveCallback_;
};
