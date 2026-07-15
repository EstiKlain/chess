#include "ClickLogger.hpp"

ClickLogger::ClickLogger(int boardRows, int boardCols, std::ostream &out)
    : boardRows_(boardRows), boardCols_(boardCols), out_(out) {}

void ClickLogger::onClick(int x, int y) const
{
    const auto cell = BoardMapper::pixelToCell(x, y, boardRows_, boardCols_);
    if (cell.has_value())
        out_ << "Clicked cell: row=" << cell->row << " col=" << cell->col << std::endl;
    else
        out_ << "Click outside board" << std::endl;
}