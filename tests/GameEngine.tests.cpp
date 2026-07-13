#include "doctest.h"

#include "GameEngine.hpp"
#include "Controller.hpp"
#include "Board.hpp"
#include "BoardPrinter.hpp"
#include "config.hpp"
#include "MoveRequest.hpp"
#include "ScriptRunner.hpp"


static pieceRules::PieceRulesRegistry registry;

namespace
{
    Board makeBoard(std::initializer_list<std::initializer_list<std::string>> rows)
    {
        Board b;
        for (const auto &row : rows)
            b.grid.push_back(std::vector<std::string>(row.begin(), row.end()));
        return b;
    }

    void clickCell(Controller &controller, int row, int col)
    {
        controller.handleClick(col * config::CELL_SIZE + config::CELL_SIZE / 2,
                               row * config::CELL_SIZE + config::CELL_SIZE / 2);
    }
}

TEST_CASE("basic_move_reaches_destination_and_origin_clears")
{
    // Why this matters: the common route must move a piece from origin to
    // destination and clear the origin once the move resolves.
    GameEngine engine(makeBoard({{"wR", ".", ".", "."}}), registry);

    MoveResult result = engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 3}});
    REQUIRE(result.accepted);

    engine.wait(3000);

    CHECK(engine.board().grid[0][3] == "wR");
    CHECK(engine.board().grid[0][0] == ".");
}

TEST_CASE("second_move_is_rejected_while_global_route_is_busy_same_color")
{
    // Why this matters: a move already in flight must block any new move
    // request, even for the same color - there is only one global route.
    GameEngine engine(makeBoard({{"wR", ".", ".", "."}, {"wP", ".", ".", "."}}), registry);

    REQUIRE(engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 3}}).accepted);

    MoveResult second = engine.requestMove(MoveRequest{Position{1, 0}, Position{1, 2}});

    CHECK_FALSE(second.accepted);
    CHECK(second.reason == "motion_in_progress");
    CHECK(engine.board().grid[1][0] == "wP");
    CHECK(engine.board().grid[1][2] == ".");
}

TEST_CASE("second_move_is_rejected_while_global_route_is_busy_opposite_color")
{
    // Why this matters: the critical invariant is that a black move cannot
    // sneak through while a white move is still in flight.
    GameEngine engine(makeBoard({{"wR", ".", ".", "."}, {"bB", ".", ".", "."}}), registry);

    REQUIRE(engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 3}}).accepted);
    engine.wait(700); // rook needs 3000ms total - still mid-flight

    MoveResult blackAttempt = engine.requestMove(MoveRequest{Position{1, 0}, Position{1, 2}});

    CHECK_FALSE(blackAttempt.accepted);
    CHECK(blackAttempt.reason == "motion_in_progress");
    CHECK(engine.board().grid[1][0] == "bB");
    CHECK(engine.board().grid[1][2] == ".");
}

TEST_CASE("move_resolves_at_exact_boundary_in_common_route")
{
    // Why this matters: the engine should resolve a move exactly at the
    // scheduled boundary, not only after it has already passed.
    GameEngine engine(makeBoard({{"wR", ".", ".", "."}}), registry);
    REQUIRE(engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 3}}).accepted);

    engine.wait(2999);
    CHECK(engine.board().grid[0][3] == ".");

    engine.wait(1);
    CHECK(engine.board().grid[0][3] == "wR");
}

TEST_CASE("can_move_again_immediately_after_arrival_with_no_cooldown")
{
    // Why this matters: once a move arrives, the piece must be immediately
    // ready for another command, with no artificial cooldown.
    GameEngine engine(makeBoard({{"wR", ".", ".", "."}}), registry);
    REQUIRE(engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 3}}).accepted);
    engine.wait(3000);

    MoveResult second = engine.requestMove(MoveRequest{Position{0, 3}, Position{0, 0}});

    CHECK(second.accepted);
    engine.wait(3000);
    CHECK(engine.board().grid[0][0] == "wR");
    CHECK(engine.board().grid[0][3] == ".");
}

TEST_CASE("second_move_to_same_destination_is_rejected_while_first_still_in_flight")
{
    // Why this matters: the global route blocks ANY second move while one
    // is in flight, including one aimed at the same destination.
    GameEngine engine(makeBoard({{"wR", ".", "bQ", "."}}), registry);
    REQUIRE(engine.requestMove(MoveRequest{Position{0, 2}, Position{0, 1}}).accepted);

    MoveResult second = engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 1}});

    CHECK_FALSE(second.accepted);
    CHECK(second.reason == "motion_in_progress");
    CHECK(engine.board().grid[0][0] == "wR");
}

TEST_CASE("illegal_move_attempt_leaves_no_phantom_motion")
{
    // Why this matters: a rejected move must not leave any leftover motion
    // state behind - the very next legal request must still succeed.
    GameEngine engine(makeBoard({{"wR", ".", "wP"}}), registry);
    MoveResult illegal = engine.requestMove(MoveRequest{Position{0, 0}, Position{1, 1}});
    CHECK_FALSE(illegal.accepted);
    CHECK(engine.board().grid[0][0] == "wR");

    // If the illegal attempt had registered a phantom motion, this would
    // now be rejected with "motion_in_progress" instead of succeeding.
    MoveResult afterwards = engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 1}});
    CHECK(afterwards.accepted);
}

TEST_CASE("capture_is_accepted_immediately_but_only_applied_on_arrival")
{
    // Why this matters: the engine must accept a legal capture request right
    // away, but the board must not change until the moving piece arrives.
    GameEngine engine(makeBoard({{"wR", ".", "bP"}}), registry);

    MoveResult result = engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 2}});

    CHECK(result.accepted);
    CHECK(engine.board().grid[0][0] == "wR"); // still at origin, mid-flight
    CHECK(engine.board().grid[0][2] == "bP"); // target not yet captured

    engine.wait(2000);
    CHECK(engine.board().grid[0][2] == "wR"); // capture applied on arrival
}

TEST_CASE("handle_wait_accumulates_elapsed_time_to_resolve_late_move")
{
    // Why this matters: cumulative waits must resolve a move at the right
    // global time, not only at the end of a single large wait.
    GameEngine engine(makeBoard({{"wR", ".", ".", "."}}), registry);
    REQUIRE(engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 3}}).accepted);

    engine.wait(200);
    engine.wait(200);
    engine.wait(200);
    CHECK(engine.board().grid[0][3] == ".");

    engine.wait(2400);
    CHECK(engine.board().grid[0][3] == "wR");
}

TEST_CASE("capturing_king_ends_game_and_blocks_further_moves")
{
    // Why this matters: once a king is captured, the game must freeze
    // immediately and later move commands must be ignored.
    GameEngine engine(makeBoard({{"wR", ".", "bK"}}), registry);
    REQUIRE(engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 2}}).accepted);
    engine.wait(2000);

    CHECK(engine.gameOver());
    CHECK(engine.board().grid[0][2] == "wR");

    MoveResult afterGameOver = engine.requestMove(MoveRequest{Position{0, 2}, Position{0, 1}});
    CHECK_FALSE(afterGameOver.accepted);
    CHECK(afterGameOver.reason == "game_over");
    CHECK(engine.board().grid[0][2] == "wR");
}

TEST_CASE("request_move_from_empty_cell_is_rejected")
{
    // Why this matters: Stage 4 - RuleEngine must reject a move requested
    // from an empty source cell before any other check runs.
    GameEngine engine(makeBoard({{"wR", ".", ".", "."}}), registry);

    MoveResult result = engine.requestMove(MoveRequest{Position{0, 1}, Position{0, 2}});

    CHECK_FALSE(result.accepted);
    CHECK(result.reason == "empty_source");
}

TEST_CASE("run_commands_respect_global_route_integration")
{
    // Why this matters: the command layer should preserve the same
    // single-route invariant from clicks and waits all the way through
    // printing - this is the only test here that goes through the full
    // click -> Controller -> GameEngine -> DSL pipeline.
    GameEngine engine(makeBoard({{"wR", ".", ".", "."}, {"bB", ".", ".", "."}}), registry);
    std::vector<std::string> commands = {
        "click 50 50",
        "click 350 50",
        "click 50 150",
        "click 250 150",
        "wait 700",
        "print board"};

    runCommands(commands, engine);

    CHECK(formatBoard(engine.board()) == "wR . . .\nbB . . .\n");
}

TEST_CASE("print_board_mid_flight_still_shows_piece_at_origin")
{
    GameEngine engine(makeBoard({{"wR", ".", ".", "."}}), registry);
    REQUIRE(engine.requestMove(MoveRequest{Position{0, 0}, Position{0, 3}}).accepted); // duration 3000ms

    engine.wait(500); // still mid-flight

    CHECK(engine.board().grid[0][0] == "wR");
    CHECK(engine.board().grid[0][3] == ".");
}

TEST_CASE("pawn_double_step_reaches_destination_after_wait")
{
    GameEngine engine(makeBoard({{".", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {"wP", ".", ".", "."},
                                 {".", ".", ".", "."}}), registry);

    REQUIRE(engine.requestMove(MoveRequest{Position{6, 0}, Position{4, 0}}).accepted);
    engine.wait(1000);

    CHECK(engine.board().grid[4][0] == "wP");
    CHECK(engine.board().grid[6][0] == ".");
}

TEST_CASE("pawn_promotion_to_queen_after_reaching_far_row")
{
    GameEngine engine(makeBoard({{".", ".", ".", "."},
                                 {"wP", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {".", ".", ".", "."},
                                 {".", ".", ".", "."}}), registry);

    REQUIRE(engine.requestMove(MoveRequest{Position{1, 0}, Position{0, 0}}).accepted);
    engine.wait(500);

    CHECK(engine.board().grid[0][0] == "wQ");
    CHECK(engine.board().grid[1][0] == ".");
}