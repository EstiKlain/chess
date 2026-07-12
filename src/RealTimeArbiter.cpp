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
        std::string &target = st.board.grid[m.toRow][m.toCol];
        std::string &origin = st.board.grid[m.fromRow][m.fromCol];

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
    return captured;
}


//will be used once multiple concurrent moves are supported
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
