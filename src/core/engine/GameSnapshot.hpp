#pragma once

#include <vector>

// Read-only, value-copy view of the game state for the UI layer.
// Built fresh by GameEngine::snapshot() on every call. Every field here
// is a plain value (int/char) - never a pointer or reference into
// Board/Piece - so once returned, a GameSnapshot stays valid even after
// the engine mutates board_ on the next tick. Renderer/HUD code (view/)
// must only ever read game state through this struct, never through
// Board/Piece directly (see kungfu_chess_ui_plan.md, UI-Iteration D).
//
// Deliberately does NOT carry selection: selection is Controller's
// (input-layer) concept, not the engine's. main_gui.cpp (composition
// root) merges GameSnapshot with Controller::hasSelection()/selectedRow()/
// selectedCol() itself when it builds what to render.
struct PieceSnapshot
{
    int id;
    char color; // 'w' or 'b'
    char kind;  // 'K','Q','R','B','N','P'
    int row;
    int col;
};

struct GameSnapshot
{
    int rows = 0;
    int cols = 0;
    std::vector<PieceSnapshot> pieces;
    bool gameOver = false;
};
