#include "BoardPrinter.hpp"

#include <sstream>

std::string formatBoard(const Board& b) {
    std::ostringstream out;
    for (const auto& row : b.grid) {
        for (size_t j = 0; j < row.size(); ++j) {
            if (j) out << ' ';
            out << row[j];
        }
        out << '\n';
    }
    return out.str();
}