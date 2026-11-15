#pragma once

#include "engine/GameSnapshot.hpp"
#include "model/Board.hpp"

/// Rebuilds `board`'s pieces to match `snapshot`: every existing piece is
/// removed first (Board has no "clear"), then re-added from the snapshot.
/// Resulting piece ids do not match the snapshot's - callers that only ever
/// look pieces up by position (never by id) can treat this as a faithful
/// mirror.
void syncBoardFromSnapshot(Board &board, const GameSnapshot &snapshot);
