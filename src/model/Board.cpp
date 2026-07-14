#include "Board.hpp"

int Board::addPiece(char color, char kind, Position cell)
{
    if (pieceAt(cell) != nullptr)
        throw BoardError("duplicate_occupancy");

    Piece p;
    p.id = nextId_++;
    p.color = color;
    p.kind = kind;
    p.cell = cell;
    p.state = PieceState::Idle;

    pieces_.push_back(p);
    idToIndex_[p.id] = static_cast<int>(pieces_.size()) - 1;
    return p.id;
}

void Board::removePiece(int id)
{
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end())
        return; // already gone - removePiece is idempotent by design

    const int idx = it->second;
    const int lastIdx = static_cast<int>(pieces_.size()) - 1;

    if (idx != lastIdx)
    {
        pieces_[idx] = pieces_[lastIdx];
        idToIndex_[pieces_[idx].id] = idx;
    }
    pieces_.pop_back();
    idToIndex_.erase(it);
}

const Piece *Board::pieceAt(Position cell) const
{
    for (const auto &p : pieces_)
        if (p.cell == cell)
            return &p;
    return nullptr;
}

Piece *Board::pieceAt(Position cell)
{
    for (auto &p : pieces_)
        if (p.cell == cell)
            return &p;
    return nullptr;
}

const Piece *Board::pieceById(int id) const
{
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end())
        return nullptr;
    return &pieces_[it->second];
}

Piece *Board::pieceById(int id)
{
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end())
        return nullptr;
    return &pieces_[it->second];
}

void Board::movePiece(int id, Position to)
{
    Piece *p = pieceById(id);
    if (!p)
        throw BoardError("unknown_piece");
    p->cell = to;
}
