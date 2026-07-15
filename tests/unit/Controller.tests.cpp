#include "doctest.h"

#include <vector>

#include "input/Controller.hpp"
#include "model/Board.hpp"
#include "io/BoardParser.hpp"
#include "rules/Movement.hpp"
#include "config.hpp"

namespace
{
    struct FakeGameEngine
    {
        std::vector<MoveRequest> requests;
        std::vector<std::pair<int, int>> jumps;

        void requestMove(const MoveRequest &request)
        {
            requests.push_back(request);
        }

        void requestJump(int row, int col)
        {
            jumps.emplace_back(row, col);
        }
    };

    Board makeBoard(std::initializer_list<std::initializer_list<std::string>> rows)
    {
        RawBoard raw;
        for (const auto &row : rows)
            raw.push_back(std::vector<std::string>(row.begin(), row.end()));
        return buildBoard(raw);
    }

    // Local helper: reads back a single cell as a "colorkind" token (or
    // ".") purely for test assertions - Board itself has no such concept.
    std::string tokenAt(const Board &b, int row, int col)
    {
        const Piece *p = b.pieceAt(Position{row, col});
        if (!p) return ".";
        return std::string(1, p->color) + std::string(1, p->kind);
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
    CHECK(tokenAt(b, 0, 0) == "wR");
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
    CHECK(tokenAt(b, 0, 0) == "wR");
    CHECK(tokenAt(b, 0, 2) == ".");
    CHECK(engine.requests.empty());
}
TEST_CASE("jump_click_outside_board_is_ignored")
{
    Board b = makeBoard({{"wR", ".", "."}});
    FakeGameEngine engine;
    Controller controller(b, engine);

    controller.handleJumpClick(-1, 50);
    controller.handleJumpClick(1000, 50);
    controller.handleJumpClick(50, 1000);

    CHECK(engine.jumps.empty());
}

TEST_CASE("jump_click_in_board_sends_correct_cell_and_does_not_touch_selection")
{
    Board b = makeBoard({{"wR", ".", "."}});
    FakeGameEngine engine;
    Controller controller(b, engine);

    clickCell(controller, 0, 0);
    CHECK(controller.hasSelection());

    controller.handleJumpClick(50 + 2 * config::CELL_SIZE, 50);

    REQUIRE(engine.jumps.size() == 1);
    CHECK(engine.jumps[0].first == 0);
    CHECK(engine.jumps[0].second == 2);

    CHECK(controller.hasSelection());
    CHECK(controller.selectedRow() == 0);
    CHECK(controller.selectedCol() == 0);
}
