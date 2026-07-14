#pragma once

#include <optional>

#include "model/Position.hpp"

namespace BoardMapper
{
    std::optional<Position> pixelToCell(int x, int y, int boardRows, int boardCols);
}
