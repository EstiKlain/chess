#include "doctest.h"

#include <nlohmann/json.hpp>

#include "engine/GameSnapshot.hpp"
#include "engine/MoveRequest.hpp"
#include "model/Position.hpp"
#include "server/protocol/dto/JumpDto.hpp"
#include "server/protocol/dto/MoveDto.hpp"
#include "server/protocol/mappers/GameSnapshotMapper.hpp"
#include "server/protocol/mappers/MoveRequestMapper.hpp"

TEST_CASE("MoveRequestMapper: domain -> DTO -> JSON -> DTO -> domain round-trips exactly") {
    const MoveRequest original{Position{1, 2}, Position{3, 4}};

    const MoveDto dto = MoveRequestMapper::toDto(original);
    const nlohmann::json json = dto;
    const MoveDto dtoBack = json.get<MoveDto>();
    const MoveRequest domainBack = MoveRequestMapper::toDomain(dtoBack);

    CHECK(domainBack.from == original.from);
    CHECK(domainBack.to == original.to);
}

TEST_CASE("JumpDto: JSON round-trips exactly, independent of MoveDto's shape") {
    const JumpDto original{5, 6};

    const nlohmann::json json = original;
    const JumpDto back = json.get<JumpDto>();

    CHECK(back.row == original.row);
    CHECK(back.col == original.col);
    // JumpDto has no "from"/"to" fields at all - it is not MoveDto in disguise.
    CHECK_FALSE(json.contains("fromRow"));
}

TEST_CASE("GameSnapshotMapper: toJson carries every GameSnapshot field plus the recipient's role") {
    GameSnapshot snapshot;
    snapshot.rows = 8;
    snapshot.cols = 8;
    snapshot.gameOver = false;
    snapshot.nowMs = 1234;

    PieceSnapshot piece{};
    piece.id = 1;
    piece.color = 'w';
    piece.kind = 'R';
    piece.row = 0;
    piece.col = 0;
    snapshot.pieces.push_back(piece);

    const std::vector<PlayerDto> players = {{"conn-w", "w", "Alice"}, {"conn-b", "b", "Bob"}};
    const nlohmann::json whiteView = GameSnapshotMapper::toJson(snapshot, 'w', players);
    const nlohmann::json blackView = GameSnapshotMapper::toJson(snapshot, 'b', players);

    CHECK(whiteView.at("rows") == 8);
    CHECK(whiteView.at("cols") == 8);
    CHECK(whiteView.at("gameOver") == false);
    CHECK(whiteView.at("nowMs") == 1234);
    REQUIRE(whiteView.at("pieces").size() == 1);
    CHECK(whiteView.at("pieces")[0].at("color") == "w");
    CHECK(whiteView.at("pieces")[0].at("kind") == "R");

    // The one field that must differ between the two recipients' payloads.
    CHECK(whiteView.at("role") == "w");
    CHECK(blackView.at("role") == "b");
}

TEST_CASE("GameSnapshotMapper: toJson carries the full players array with id/color/name, identical for every recipient") {
    GameSnapshot snapshot;
    snapshot.rows = 8;
    snapshot.cols = 8;

    const std::vector<PlayerDto> players = {{"conn-w", "w", "Alice"}, {"conn-b", "b", "Bob"}};
    const nlohmann::json whiteView = GameSnapshotMapper::toJson(snapshot, 'w', players);
    const nlohmann::json blackView = GameSnapshotMapper::toJson(snapshot, 'b', players);

    REQUIRE(whiteView.at("players").size() == 2);
    CHECK(whiteView.at("players")[0].at("id") == "conn-w");
    CHECK(whiteView.at("players")[0].at("color") == "w");
    CHECK(whiteView.at("players")[0].at("name") == "Alice");

    // The players array itself does not vary per recipient - only the
    // top-level "role" does.
    CHECK(whiteView.at("players") == blackView.at("players"));
}
