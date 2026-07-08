#pragma once

#include <string>
#include <vector>

#include "Board.hpp"

struct Selection {
    bool active = false;
    int  row = 0, col = 0;
    long selectedAtMs = 0;
};

struct PieceMove {
    int         fromRow, fromCol;
    int         toRow, toCol;
    long        startMs;
    long        durationMs;
    std::string piece;
};

struct GameState {
    Board                  board;
    long                   elapsedMs = 0;
    std::vector<Selection> selections;
    std::vector<PieceMove> activeMoves;
};
