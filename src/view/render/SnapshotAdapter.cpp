#include "SnapshotAdapter.hpp"

#include <cctype>

namespace SnapshotAdapter
{
    std::vector<PiecePlacement> toPlacements(const GameSnapshot &snapshot)
    {
        std::vector<PiecePlacement> placements;
        placements.reserve(snapshot.pieces.size());

        for (const PieceSnapshot &p : snapshot.pieces)
        {
            const std::string code = std::string(1, p.kind) +
                                      std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(p.color))));
            placements.push_back(PiecePlacement{code, p.row, p.col});
        }

        return placements;
    }
}
