#include "doctest.h"

#include "rules/CollisionRule.hpp"

namespace
{
    PieceMove makeMove(int pieceId, long startMs, long durationMs, std::vector<Position> path)
    {
        PieceMove m{};
        m.fromRow = 0;
        m.fromCol = 0;
        m.toRow = path.empty() ? 0 : path.back().row;
        m.toCol = path.empty() ? 0 : path.back().col;
        m.startMs = startMs;
        m.durationMs = durationMs;
        m.pieceId = pieceId;
        m.path = std::move(path);
        return m;
    }

    Piece makePiece(int id, char color)
    {
        return Piece{id, color, 'R', Position{0, 0}, PieceState::Moving};
    }
}

TEST_CASE("decideCollisionOutcome resolves an exact tie between different colors as a mutual capture")
{
    // Same colliding cell reached at the exact same instant by both movers -
    // neither is "later", so this is a symmetric head-on collision: both
    // are captured, not left undecided.
    PieceMove a = makeMove(1, 0, 400, {Position{0, 1}, Position{0, 2}});
    PieceMove b = makeMove(2, 0, 400, {Position{1, 1}, Position{0, 2}});
    Piece moverA = makePiece(1, 'w');
    Piece moverB = makePiece(2, 'b');
    CellCollision collision{1, 1, 400, 400};

    std::vector<CollisionDecision> decisions = decideCollisionOutcome(a, moverA, b, moverB, collision);

    REQUIRE(decisions.size() == 2);
    CHECK(decisions[0].kind == CollisionDecision::Kind::Capture);
    CHECK(decisions[0].pieceId == moverA.id);
    CHECK(decisions[1].kind == CollisionDecision::Kind::Capture);
    CHECK(decisions[1].pieceId == moverB.id);
}

TEST_CASE("decideCollisionOutcome resolves an exact tie between same-color movers as both stopping short")
{
    PieceMove a = makeMove(1, 0, 400, {Position{0, 1}, Position{0, 2}});
    PieceMove b = makeMove(2, 0, 400, {Position{1, 1}, Position{0, 2}});
    Piece moverA = makePiece(1, 'w');
    Piece moverB = makePiece(2, 'w');
    CellCollision collision{1, 1, 400, 400};

    std::vector<CollisionDecision> decisions = decideCollisionOutcome(a, moverA, b, moverB, collision);

    REQUIRE(decisions.size() == 2);
    CHECK(decisions[0].pieceId == moverA.id);
    CHECK(decisions[1].pieceId == moverB.id);
    // Both movers' shared cell sits at path index 1 for each of them, so
    // both truncate to their own first cell (index 0), not cancel.
    CHECK(decisions[0].kind == CollisionDecision::Kind::Truncate);
    CHECK(decisions[1].kind == CollisionDecision::Kind::Truncate);
    REQUIRE(decisions[0].newPath.size() == 1);
    CHECK(decisions[0].newPath[0] == Position{0, 1});
    REQUIRE(decisions[1].newPath.size() == 1);
    CHECK(decisions[1].newPath[0] == Position{1, 1});
}

TEST_CASE("decideCollisionOutcome captures mover A when A arrives first and colors differ")
{
    PieceMove a = makeMove(1, 0, 300, {Position{0, 1}, Position{0, 2}});
    PieceMove b = makeMove(2, 0, 500, {Position{1, 1}, Position{0, 2}});
    Piece moverA = makePiece(1, 'w');
    Piece moverB = makePiece(2, 'b');
    CellCollision collision{1, 1, 300, 500}; // A earlier

    std::vector<CollisionDecision> decisions = decideCollisionOutcome(a, moverA, b, moverB, collision);

    REQUIRE(decisions.size() == 1);
    CHECK(decisions[0].kind == CollisionDecision::Kind::Capture);
    CHECK(decisions[0].pieceId == moverA.id); // the earlier arrival is captured
}

TEST_CASE("decideCollisionOutcome captures mover B when B arrives first and colors differ")
{
    PieceMove a = makeMove(1, 0, 500, {Position{0, 1}, Position{0, 2}});
    PieceMove b = makeMove(2, 0, 300, {Position{1, 1}, Position{0, 2}});
    Piece moverA = makePiece(1, 'w');
    Piece moverB = makePiece(2, 'b');
    CellCollision collision{1, 1, 500, 300}; // B earlier

    std::vector<CollisionDecision> decisions = decideCollisionOutcome(a, moverA, b, moverB, collision);

    REQUIRE(decisions.size() == 1);
    CHECK(decisions[0].kind == CollisionDecision::Kind::Capture);
    CHECK(decisions[0].pieceId == moverB.id); // the earlier arrival is captured
}

TEST_CASE("decideCollisionOutcome cancels the later same-color mover when the shared cell is its very first path cell")
{
    // indexA == 0 -> the shared cell is A's first step, so a later A never
    // even takes a single step before being stopped.
    PieceMove a = makeMove(1, 0, 300, {Position{0, 2}});
    PieceMove b = makeMove(2, 0, 200, {Position{1, 1}, Position{0, 2}});
    Piece moverA = makePiece(1, 'w');
    Piece moverB = makePiece(2, 'w');
    CellCollision collision{0, 1, 300, 220}; // A is later (300 > 220), collides at A's index 0

    std::vector<CollisionDecision> decisions = decideCollisionOutcome(a, moverA, b, moverB, collision);

    REQUIRE(decisions.size() == 1);
    CHECK(decisions[0].kind == CollisionDecision::Kind::Cancel);
    CHECK(decisions[0].pieceId == moverA.id);
}

TEST_CASE("decideCollisionOutcome cancels mover B symmetrically when B is later and collides on its own first cell")
{
    PieceMove a = makeMove(1, 0, 200, {Position{1, 1}, Position{0, 2}});
    PieceMove b = makeMove(2, 0, 300, {Position{0, 2}});
    Piece moverA = makePiece(1, 'b');
    Piece moverB = makePiece(2, 'b');
    CellCollision collision{1, 0, 220, 300}; // B is later (300 > 220), collides at B's index 0

    std::vector<CollisionDecision> decisions = decideCollisionOutcome(a, moverA, b, moverB, collision);

    REQUIRE(decisions.size() == 1);
    CHECK(decisions[0].kind == CollisionDecision::Kind::Cancel);
    CHECK(decisions[0].pieceId == moverB.id);
}

TEST_CASE("decideCollisionOutcome truncates the later same-color mover one cell short, along its own path")
{
    // A's own path has 7 cells; the shared cell sits at A's index 3 (the
    // 4th cell) - A should be truncated to its own first 3 cells.
    std::vector<Position> pathA = {Position{1, 2}, Position{2, 2}, Position{3, 2},
                                    Position{4, 2}, Position{5, 2}, Position{6, 2}, Position{7, 2}};
    std::vector<Position> pathB = {Position{4, 0}, Position{4, 2}};
    PieceMove a = makeMove(1, 0, 700, pathA);
    PieceMove b = makeMove(2, 220, 700, pathB);
    Piece moverA = makePiece(1, 'w');
    Piece moverB = makePiece(2, 'w');
    CellCollision collision{3, 1, 400, 420}; // B is later (420 > 400)

    std::vector<CollisionDecision> decisions = decideCollisionOutcome(a, moverA, b, moverB, collision);

    REQUIRE(decisions.size() == 1);
    CHECK(decisions[0].kind == CollisionDecision::Kind::Truncate);
    CHECK(decisions[0].pieceId == moverB.id);
    REQUIRE(decisions[0].newPath.size() == 1);
    CHECK(decisions[0].newPath[0] == pathB[0]);
    CHECK(decisions[0].newDurationMs == (700 * 1) / 2);
}

TEST_CASE("decideCollisionOutcome truncates mover A one cell short when A is the later same-color mover")
{
    std::vector<Position> pathA = {Position{1, 2}, Position{2, 2}, Position{3, 2},
                                    Position{4, 2}, Position{5, 2}, Position{6, 2}, Position{7, 2}};
    std::vector<Position> pathB = {Position{4, 0}, Position{4, 2}};
    PieceMove a = makeMove(1, 220, 700, pathA);
    PieceMove b = makeMove(2, 0, 700, pathB);
    Piece moverA = makePiece(1, 'b');
    Piece moverB = makePiece(2, 'b');
    CellCollision collision{3, 1, 620, 400}; // A later (620 > 400)

    std::vector<CollisionDecision> decisions = decideCollisionOutcome(a, moverA, b, moverB, collision);

    REQUIRE(decisions.size() == 1);
    CHECK(decisions[0].kind == CollisionDecision::Kind::Truncate);
    CHECK(decisions[0].pieceId == moverA.id);
    REQUIRE(decisions[0].newPath.size() == 3);
    CHECK(decisions[0].newPath[0] == pathA[0]);
    CHECK(decisions[0].newPath[1] == pathA[1]);
    CHECK(decisions[0].newPath[2] == pathA[2]);
    CHECK(decisions[0].newDurationMs == (700 * 3) / 7);
}
