#include "BoardPrinter.hpp"

#include <sstream>
#include <vector>

std::string formatBoard(const Board& b) {
    // Build a printable text grid purely for output purposes - this is
    // local/temporary and does not change the text I/O format at all.
    std::vector<std::vector<std::string>> grid(
        b.height, std::vector<std::string>(b.width, "."));

    for (const auto& p : b.pieces())
        grid[p.cell.row][p.cell.col] = std::string(1, p.color) + std::string(1, p.kind);

    std::ostringstream out;
    for (const auto& row : grid) {
        for (size_t j = 0; j < row.size(); ++j) {
            if (j) out << ' ';
            out << row[j];
        }
        out << '\n';
    }
    return out.str();
}
