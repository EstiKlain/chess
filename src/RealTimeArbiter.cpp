#include "RealTimeArbiter.hpp"

#include <algorithm>

#include "Board.hpp"
#include "Movement.hpp"
#include "config.hpp"
std::vector<std::string> resolveMoves(GameState &st)
{
    std::vector<std::string> captured;
    std::vector<size_t> due;
    std::vector<PieceMove> stillMoving;
    for (size_t i = 0; i < st.activeMoves.size(); ++i)
    {
        const PieceMove &m = st.activeMoves[i];
        if (st.elapsedMs >= m.startMs + m.durationMs)
            due.push_back(i);
        else
            stillMoving.push_back(m);
    }

    std::sort(due.begin(), due.end(), [&](size_t a, size_t b)
              {
        long ta = st.activeMoves[a].startMs + st.activeMoves[a].durationMs;
        long tb = st.activeMoves[b].startMs + st.activeMoves[b].durationMs;
        if (ta != tb) return ta < tb;
        return a < b; });

    for (size_t idx : due)
    {
        const PieceMove &m = st.activeMoves[idx];
        long arrivalMs = m.startMs + m.durationMs;
        std::string &target = st.board.grid[m.toRow][m.toCol];
        std::string &origin = st.board.grid[m.fromRow][m.fromCol];

        bool reverseCaptured = false;
        for (const auto &j : st.activeJumps)
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
                st.board.grid[m.fromRow][m.fromCol] = ".";
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
            if (pieceOf(m.piece) == 'P' && m.toRow == config::pawnPromotionRow(colorOf(m.piece), st.board.rows()))
                target[1] = 'Q';
            origin = ".";
        }
        else if (isEmpty(target))
        {
            target = m.piece;
            if (pieceOf(m.piece) == 'P' && m.toRow == config::pawnPromotionRow(colorOf(m.piece), st.board.rows()))
                target[1] = 'Q';
            origin = ".";
        }
        // else: friendly piece blocks destination -> move fails, piece stays at origin
        // (origin was never cleared, so nothing to do here)
    }

    st.activeMoves = stillMoving;

    std::vector<JumpMove> stillJumping;
    for (const auto &j : st.activeJumps)
    {
        if (st.elapsedMs >= j.startMs + j.durationMs)
            continue; // landed, drop it
        stillJumping.push_back(j);
    }
    st.activeJumps = stillJumping;
    return captured;
}

// will be used once multiple concurrent moves are supported
bool isPieceInFlight(const GameState &st, int row, int col)
{
    for (const PieceMove &move : st.activeMoves)
    {
        if (move.fromRow == row && move.fromCol == col)
        {
            return true;
        }
    }
    return false;
}

bool hasActiveMotion(const GameState &st)
{
    return !st.activeMoves.empty();
}
