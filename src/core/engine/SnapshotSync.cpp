#include "engine/SnapshotSync.hpp"

void syncBoardFromSnapshot(Board &board, const GameSnapshot &snapshot)
{
    board.width = snapshot.cols;
    board.height = snapshot.rows;

    const std::vector<Piece> existing = board.pieces();
    for (const Piece &piece : existing)
    {
        board.removePiece(piece.id);
    }
    for (const PieceSnapshot &piece : snapshot.pieces)
    {
        board.addPiece(piece.color, piece.kind, Position{piece.row, piece.col});
    }
}
