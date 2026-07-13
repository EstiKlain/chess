#include "doctest.h"

#include <vector>

#include "Controller.hpp"
#include "Board.hpp"
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

    Board makeBoard(std::initializer_list<std::initializer_list<std::string>> rows)
    {
        Board b;
        for (const auto &row : rows)
        {
            b.grid.push_back(std::vector<std::string>(row.begin(), row.end()));
        }
        return b;
    }

    void clickCell(Controller &controller, int row, int col)
    {
        controller.handleClick(col * config::CELL_SIZE + config::CELL_SIZE / 2,
                               row * config::CELL_SIZE + config::CELL_SIZE / 2);
    }
}

TEST_CASE("selection_resets_immediately_after_second_click_regardless_of_legality")
{
    Board b = makeBoard({{"wR", ".", "."}});
    FakeGameEngine engine;
    Controller controller(b, engine);

    controller.handleClick(50, 50);
    clickCell(controller, 0, 2);

    CHECK_FALSE(controller.hasSelection());
    REQUIRE(engine.requests.size() == 1);
    CHECK(engine.requests[0].from.row == 0);
    CHECK(engine.requests[0].from.col == 0);
    CHECK(engine.requests[0].to.row == 0);
    CHECK(engine.requests[0].to.col == 2);
}

TEST_CASE("reselecting_same_color_piece_updates_selection_without_move")
{
    Board b = makeBoard({{"wR", ".", "wP", "."}});
    FakeGameEngine engine;
    Controller controller(b, engine);

    controller.handleClick(50, 50);
    clickCell(controller, 0, 2);

    CHECK(controller.hasSelection());
    CHECK(controller.selectedRow() == 0);
    CHECK(controller.selectedCol() == 2);
    CHECK(engine.requests.empty());
}

TEST_CASE("send_move_rejects_when_selection_is_inactive")
{
    Board b = makeBoard({{"wR", ".", "."}});
    FakeGameEngine engine;
    Controller controller(b, engine);

    controller.requestMove({{0, 0}, {0, 2}});

    CHECK_FALSE(controller.hasSelection());
    CHECK(b.grid[0][0] == "wR");
    CHECK(engine.requests.empty());
}

TEST_CASE("invalid_clicks_do_not_mutate_state")
{
    Board b = makeBoard({{"wR", ".", "."}});
    FakeGameEngine engine;
    Controller controller(b, engine);

    controller.handleClick(-1, 50);
    controller.handleClick(1000, 50);
    controller.handleClick(50, 1000);
    clickCell(controller, 0, 2);

    CHECK_FALSE(controller.hasSelection());
    CHECK(b.grid[0][0] == "wR");
    CHECK(b.grid[0][2] == ".");
    CHECK(engine.requests.empty());
}
