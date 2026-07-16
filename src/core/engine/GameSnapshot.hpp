#pragma once

#include <optional>
#include <vector>
#include "model/Piece.hpp"
struct MotionSnapshot
{
    int fromRow, fromCol;
    int toRow, toCol;
    long startMs;
    long durationMs;
};
struct PieceSnapshot
{
    int id;
    char color;
    char kind;
    int row;
    int col;

    PieceState state = PieceState::Idle;
    long stateStartMs = 0;
    long stateDurationMs = 0;
    std::optional<MotionSnapshot> motion;
};

struct GameSnapshot
{
    int rows = 0;
    int cols = 0;
    std::vector<PieceSnapshot> pieces;
    bool gameOver = false;
    long nowMs = 0;
};
