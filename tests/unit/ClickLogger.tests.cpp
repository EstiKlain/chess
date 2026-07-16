#include "doctest.h"

#include <sstream>

#include "view/input/ClickLogger.hpp"

TEST_CASE("click_logger_prints_cell_for_in_board_click")
{
    std::ostringstream out;
    ClickLogger logger(4, 4, out);

    logger.onClick(150, 250);

    CHECK(out.str() == "Clicked cell: row=2 col=1\n");
}

TEST_CASE("click_logger_prints_outside_message_for_out_of_board_click")
{
    std::ostringstream out;
    ClickLogger logger(4, 4, out);

    logger.onClick(-5, 10);

    CHECK(out.str() == "Click outside board\n");
}