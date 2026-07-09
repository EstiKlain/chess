#include "doctest.h"

#include <vector>

#include "Controller.hpp"
#include "GameState.hpp"
#include "Movement.hpp"
#include "config.hpp"

namespace
{
    struct FakeGameEngine
    {
        std::vector<MoveRequest> requests;

        void requestMove(const MoveRequest &request)
        {
            requests.push_back(request);
        }
    };

    GameState makeState(std::initializer_list<std::initializer_list<std::string>> rows)
    {
        GameState st;
        for (const auto &row : rows)
        {
            st.board.grid.push_back(std::vector<std::string>(row.begin(), row.end()));
        }
        return st;
    }

    void clickCell(Controller &controller, int row, int col)
    {
        controller.handleClick(col * config::CELL_SIZE + config::CELL_SIZE / 2,
                               row * config::CELL_SIZE + config::CELL_SIZE / 2);
    }
}

TEST_CASE("selection_resets_immediately_after_second_click_regardless_of_legality")
{
    GameState st = makeState({{"wR", ".", "."}});
    FakeGameEngine engine;
    Controller controller(st, engine);

    controller.handleClick(50, 50);
    clickCell(controller, 0, 2);

    CHECK_FALSE(st.selection.active);
    CHECK(st.activeMoves.empty());
    REQUIRE(engine.requests.size() == 1);
    CHECK(engine.requests[0].from.row == 0);
    CHECK(engine.requests[0].from.col == 0);
    CHECK(engine.requests[0].to.row == 0);
    CHECK(engine.requests[0].to.col == 2);
}

TEST_CASE("reselecting_same_color_piece_updates_selection_without_move")
{
    GameState st = makeState({{"wR", ".", "wP", "."}});
    FakeGameEngine engine;
    Controller controller(st, engine);

    controller.handleClick(50, 50);
    clickCell(controller, 0, 2);

    CHECK(st.selection.active);
    CHECK(st.selection.row == 0);
    CHECK(st.selection.col == 2);
    CHECK(st.activeMoves.empty());
    CHECK(engine.requests.empty());
}

TEST_CASE("send_move_rejects_when_selection_is_inactive")
{
    GameState st = makeState({{"wR", ".", "."}});
    FakeGameEngine engine;
    Controller controller(st, engine);

    controller.requestMove({{0, 0}, {0, 2}});

    CHECK(st.activeMoves.empty());
    CHECK_FALSE(st.selection.active);
    CHECK(st.board.grid[0][0] == "wR");
    CHECK(engine.requests.empty());
}

TEST_CASE("invalid_clicks_do_not_mutate_state")
{
    GameState st = makeState({{"wR", ".", "."}});
    FakeGameEngine engine;
    Controller controller(st, engine);

    controller.handleClick(-1, 50);
    controller.handleClick(1000, 50);
    controller.handleClick(50, 1000);
    clickCell(controller, 0, 2);

    CHECK_FALSE(st.selection.active);
    CHECK(st.activeMoves.empty());
    CHECK(st.board.grid[0][0] == "wR");
    CHECK(st.board.grid[0][2] == ".");
    CHECK(engine.requests.empty());
}
