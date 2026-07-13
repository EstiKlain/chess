#include "RealTimeArbiter.hpp"

#include <algorithm>

#include "Board.hpp"
#include "Movement.hpp"
#include "PieceRules.hpp"

std::vector<std::string> RealTimeArbiter::resolveMoves(Board &board, long elapsedMs, const pieceRules::PieceRulesRegistry &registry)
{
    std::vector<std::string> captured;
    std::vector<size_t> due;
    std::vector<PieceMove> stillMoving;
    for (size_t i = 0; i < activeMoves_.size(); ++i)
    {
        const PieceMove &m = activeMoves_[i];
        if (elapsedMs >= m.startMs + m.durationMs)
            due.push_back(i);
        else
            stillMoving.push_back(m);
    }

    std::sort(due.begin(), due.end(), [&](size_t a, size_t b)
              {
        long ta = activeMoves_[a].startMs + activeMoves_[a].durationMs;
        long tb = activeMoves_[b].startMs + activeMoves_[b].durationMs;
        if (ta != tb) return ta < tb;
        return a < b; });

    for (size_t idx : due)
    {
        const PieceMove &m = activeMoves_[idx];
        long arrivalMs = m.startMs + m.durationMs;
        std::string &target = board.grid[m.toRow][m.toCol];
        std::string &origin = board.grid[m.fromRow][m.fromCol];

        bool reverseCaptured = false;
        for (const auto &j : activeJumps_)
        {
            if (j.row != m.toRow || j.col != m.toCol)
                continue;
            if (colorOf(j.piece) == m.piece[0])
                continue; // only an enemy jump defends

            long jumpEndMs = j.startMs + j.durationMs;
            if (arrivalMs <= jumpEndMs)
            {
                // the jumper wins: the arriving piece is captured,
                // the jumper stays exactly where it was (rule 2)
                captured.push_back(m.piece);
                board.grid[m.fromRow][m.fromCol] = ".";
                reverseCaptured = true;
            }
            break; // at most one jump can occupy a given cell
        }
        if (reverseCaptured)
            continue;

        if (!isEmpty(target) && colorOf(target) != m.piece[0])
        {
            captured.push_back(target);
            target = m.piece;
            if (pieceOf(m.piece) == 'P' && m.toRow == registry.pawnPromotionRow(colorOf(m.piece), board.rows()))
                target[1] = 'Q';
            origin = ".";
        }
        else if (isEmpty(target))
        {
            target = m.piece;
            if (pieceOf(m.piece) == 'P' && m.toRow == registry.pawnPromotionRow(colorOf(m.piece), board.rows()))
                target[1] = 'Q';
            origin = ".";
        }
        // else: friendly piece blocks destination -> move fails, piece stays at origin
        // (origin was never cleared, so nothing to do here)
    }

    activeMoves_ = stillMoving;

    std::vector<JumpMove> stillJumping;
    for (const auto &j : activeJumps_)
    {
        if (elapsedMs >= j.startMs + j.durationMs)
            continue; // landed, drop it
        stillJumping.push_back(j);
    }
    activeJumps_ = stillJumping;
    return captured;
}

bool RealTimeArbiter::isPieceInFlight(int row, int col) const
{
    for (const PieceMove &move : activeMoves_)
    {
        if (move.fromRow == row && move.fromCol == col)
        {
            return true;
        }
    }
    return false;
}

bool RealTimeArbiter::hasActiveMotion() const
{
    return !activeMoves_.empty();
}

void RealTimeArbiter::startMotion(const PieceMove &move)
{
    activeMoves_.push_back(move);
}

bool RealTimeArbiter::hasActiveJumpAt(int row, int col) const
{
    for (const auto &j : activeJumps_)
        if (j.row == row && j.col == col)
            return true;
    return false;
}

void RealTimeArbiter::startJump(const JumpMove &jump)
{
    activeJumps_.push_back(jump);
}
