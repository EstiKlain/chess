#pragma once

#include <ostream>
#include <iostream>

#include "input/BoardMapper.hpp"

class ClickLogger
{
public:
    ClickLogger(int boardRows, int boardCols, std::ostream &out = std::cout);

    void onClick(int x, int y) const;

private:
    int boardRows_;
    int boardCols_;
    std::ostream &out_;
};