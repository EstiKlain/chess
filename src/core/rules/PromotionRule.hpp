#pragma once

#include "rules/PieceRules.hpp"

/// If a piece of `kind`/`color` arriving at `arrivalRow` is a pawn reaching
/// its promotion row, returns the promoted kind ('Q'); otherwise returns
/// `kind` unchanged. Mirrors GameOverRule.hpp's/CaptureRule.hpp's pattern -
/// RealTimeArbiter calls this but never decides the promoted kind itself.
char promotedKind(char kind, char color, int arrivalRow, int totalRows,
                  const pieceRules::PieceRulesRegistry &registry);
