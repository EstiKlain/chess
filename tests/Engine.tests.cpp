#include "doctest.h"

#include "Engine.hpp"
#include "Board.hpp"
#include "GameState.hpp"
#include "Movement.hpp"
#include "config.hpp"

namespace
{
    GameState makeState(std::initializer_list<std::initializer_list<std::string>> rows)
    {
        GameState st;
        for (const auto &row : rows)
        {
            st.board.grid.push_back(std::vector<std::string>(row.begin(), row.end()));
        }
        return st;
    }

    void clickCell(GameState &st, int row, int col)
    {
        handleClick(st, col * config::CELL_SIZE + config::CELL_SIZE / 2,
                    row * config::CELL_SIZE + config::CELL_SIZE / 2);
    }
}

TEST_CASE("basic_move_reaches_destination_and_origin_clears")
{
    // Why this matters: the common route must move a piece from origin to destination and make it selectable again once the move resolves.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};

    // Act
    clickCell(st, 0, 3);
    handleWait(st, 1000);

    // Assert
    CHECK(st.board.grid[0][3] == "wR");
    CHECK(st.board.grid[0][0] == ".");
}

TEST_CASE("same_color_click_during_move_is_rejected_by_global_route")
{
    // Why this matters: a move already in flight must block any new move, even for the same color, so there is only one global route.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}, {"wP", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};
    clickCell(st, 0, 3);

    // Act
    clickCell(st, 1, 0);
    clickCell(st, 1, 2);

    // Assert
    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.board.grid[1][0] == "wP");
    CHECK(st.board.grid[1][2] == ".");
}

TEST_CASE("opposite_colors_do_not_move_concurrently_in_common_route")
{
    // Why this matters: the critical invariant is that a black click cannot sneak through while a white move is still in flight.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}, {"bB", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};
    clickCell(st, 0, 3);

    // Act
    clickCell(st, 1, 0);
    clickCell(st, 1, 2);
    handleWait(st, 700);

    // Assert
    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.board.grid[1][0] == "bB");
    CHECK(st.board.grid[1][2] == ".");
}

TEST_CASE("no_cooldown_state_in_common_route")
{
    // Why this matters: once a move arrives, the piece should be immediately ready for another command with no artificial cooldown.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};
    clickCell(st, 0, 3);
    handleWait(st, 1000);

    // Act
    clickCell(st, 0, 3);

    // Assert
    CHECK(st.selection.active);
    CHECK(st.selection.row == 0);
    CHECK(st.selection.col == 3);
    CHECK(st.activeMoves.empty());
}

TEST_CASE("can_move_again_after_arrival_without_cooldown")
{
    // Why this matters: the zero-cooldown path should allow a second move to be queued immediately after the first one lands.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};
    clickCell(st, 0, 3);
    handleWait(st, 1000);

    // Act
    clickCell(st, 0, 3);
    clickCell(st, 0, 0);

    // Assert
    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.activeMoves[0].fromRow == 0);
    CHECK(st.activeMoves[0].fromCol == 3);
    CHECK(st.activeMoves[0].toRow == 0);
    CHECK(st.activeMoves[0].toCol == 0);
}

TEST_CASE("piece_is_ready_after_arrival_without_cooldown")
{
    // Why this matters: a landed piece should not be left in a stale selection state that blocks its next move.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};
    clickCell(st, 0, 3);
    handleWait(st, 1000);

    // Act
    clickCell(st, 0, 3);
    clickCell(st, 0, 0);

    // Assert
    CHECK(st.board.grid[0][3] == ".");
    CHECK(st.board.grid[0][0] == ".");
    CHECK(st.selection.active == false);
}

TEST_CASE("move_resolves_at_exact_boundary_in_common_route")
{
    // Why this matters: the engine should resolve a move exactly at the scheduled boundary, not only after it has already passed.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};
    clickCell(st, 0, 3);

    // Act
    handleWait(st, 999);

    // Assert
    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.board.grid[0][3] == ".");

    // Act
    handleWait(st, 1);

    // Assert
    CHECK(st.activeMoves.empty());
    CHECK(st.board.grid[0][3] == "wR");
}

TEST_CASE("is_piece_in_flight_matches_origin_square_state_during_move")
{
    // Why this matters: the origin square should be treated as occupied by an in-flight piece until the move lands, matching the current engine flow.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};
    clickCell(st, 0, 3);

    // Act / Assert
    CHECK(isPieceInFlight(st, 0, 0));
    CHECK(isEmpty(st.board.grid[0][0]));
    CHECK_FALSE(isPieceInFlight(st, 0, 3));
}

TEST_CASE("send_move_rejects_when_selection_is_inactive")
{
    // Additional edge case: a disabled selection should never start a move, even if the board coordinates look legal.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    st.selection = {};

    // Act
    sendMove(st, 0, 3);

    // Assert
    CHECK(st.activeMoves.empty());
    CHECK_FALSE(st.selection.active);
    CHECK(st.board.grid[0][0] == "wR");
}

TEST_CASE("send_move_rejects_when_destination_is_already_target_of_in_flight_move")
{
    // Additional edge case: a second move should not be accepted while another move is already in flight, even if it targets that in-flight destination.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};
    PieceMove inFlight;
    inFlight.fromRow = 0;
    inFlight.fromCol = 0;
    inFlight.toRow = 0;
    inFlight.toCol = 2;
    inFlight.startMs = 0;
    inFlight.durationMs = 1000;
    inFlight.piece = "bQ";
    st.activeMoves.push_back(inFlight);

    // Act
    sendMove(st, 0, 2);

    // Assert
    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.selection.active == false);
    CHECK(st.board.grid[0][0] == "wR");
}

TEST_CASE("illegal_move_attempt_resets_selection_and_keeps_queue_empty")
{
    // Additional edge case: an illegal move must not leave behind a stale selection or a phantom active move.
    // Arrange
    GameState st = makeState({{"wR", "wP"}});
    st.selection = {true, 0, 0, st.elapsedMs};

    // Act
    sendMove(st, 0, 1);

    // Assert
    CHECK(st.activeMoves.empty());
    CHECK_FALSE(st.selection.active);
    CHECK(st.board.grid[0][0] == "wR");
}

TEST_CASE("clicking_opposite_color_piece_during_selection_attempts_capture")
{
    // Why this matters: the engine must still allow a legal capture attempt when a different-colored piece is clicked while a selection is active.
    // Arrange
    GameState st = makeState({{"wR", ".", "bP"}});
    st.selection = {true, 0, 0, st.elapsedMs};

    // Act
    clickCell(st, 0, 2);

    // Assert
    REQUIRE(st.activeMoves.size() == 1);
    CHECK(st.activeMoves[0].toRow == 0);
    CHECK(st.activeMoves[0].toCol == 2);
    CHECK(st.board.grid[0][0] == ".");
}

TEST_CASE("handle_wait_accumulates_elapsed_time_to_resolve_late_move")
{
    // Why this matters: cumulative waits must resolve a move at the right global time, not only at the end of a single large wait.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}});
    st.selection = {true, 0, 0, st.elapsedMs};
    clickCell(st, 0, 3);

    // Act
    handleWait(st, 200);
    handleWait(st, 200);
    handleWait(st, 200);

    // Assert
    REQUIRE(st.activeMoves.size() == 1);

    // Act
    handleWait(st, 400);

    // Assert
    CHECK(st.activeMoves.empty());
    CHECK(st.board.grid[0][3] == "wR");
}

TEST_CASE("run_commands_respect_global_route_integration")
{
    // Why this matters: the command layer should preserve the same single-route invariant from clicks and waits all the way through printing.
    // Arrange
    GameState st = makeState({{"wR", ".", ".", "."}, {"bB", ".", ".", "."}});
    std::vector<std::string> commands = {
        "click 50 50",
        "click 350 50",
        "click 50 150",
        "click 250 150",
        "wait 700",
        "print board"};

    // Act
    runCommands(commands, st);

    // Assert
    CHECK(formatBoard(st.board) == ". . . .\nbB . . .\n");
}
