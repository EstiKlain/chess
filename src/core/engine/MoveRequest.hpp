#pragma once

#include "model/Position.hpp"

struct MoveRequest
{
    Position from;
    Position to;

    // '\0' means "no owner asserted" - GameEngine skips the ownership check
    // for this request (used by ScriptRunner/local tooling that has no
    // player concept at all). A real caller-supplied color ('w'/'b') is
    // checked against the piece's own color.
    char requesterColor = '\0';
};
