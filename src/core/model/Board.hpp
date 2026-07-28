#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Piece.hpp"
#include "Position.hpp"

class BoardError : public std::runtime_error
{
public:
    explicit BoardError(const std::string &code)
        : std::runtime_error(code), code_(code) {}
    const std::string &code() const { return code_; }

private:
    std::string code_;
};

// Board owns the logical arrangement of pieces. It knows what exists;
// it does not decide which chess moves are legal (that's RuleEngine's
// job) and it never calls into RuleEngine itself.
class Board
{
public:
    int width = 0;
    int height = 0;

    int rows() const { return height; }
    int cols() const { return width; }

    bool inBounds(Position p) const
    {
        return p.row >= 0 && p.row < height && p.col >= 0 && p.col < width;
    }

    // Adds a new piece with a fresh, stable id. Throws BoardError
    // ("duplicate_occupancy") if the destination cell is already
    // occupied.
    int addPiece(char color, char kind, Position cell);

    // Removing an id that no longer exists is a no-op (idempotent),
    // since resolveMoves may legitimately try to remove a piece more
    // than once in edge-case orderings.
    void removePiece(int id);

    const Piece *pieceAt(Position cell) const;
    Piece *pieceAt(Position cell);

    const Piece *pieceById(int id) const;
    Piece *pieceById(int id);

    // Assumes the destination is chess-legal per RuleEngine - Board does not
    // know piece movement rules and never re-checks legality here. It does
    // assert its own structural invariants (bounds, no landing on an
    // occupied cell) in debug builds, the same invariants addPiece() already
    // enforces - not new game-rule knowledge, just applied consistently.
    void movePiece(int id, Position to);

    const std::vector<Piece> &pieces() const { return pieces_; }

private:
    std::vector<Piece> pieces_;
    std::unordered_map<int, int> idToIndex_; 
    int nextId_ = 1;
};
