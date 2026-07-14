#pragma once

#include "Position.hpp"

enum class PieceState { Idle, Moving, Captured };

struct Piece {
    int id;
    char color;   // 'w' or 'b'
    char kind;    // 'K','Q','R','B','N','P'
    Position cell;
    PieceState state = PieceState::Idle;
};
