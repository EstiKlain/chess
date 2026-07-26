#pragma once

#include <vector>

#include <nlohmann/json.hpp>

#include "engine/GameSnapshot.hpp"
#include "server/protocol/dto/StateUpdateDto.hpp"

// The other core/<->protocol/ meeting point (alongside MoveRequestMapper).
// Takes the recipient's color explicitly: GameSnapshot itself is shared
// between both players in a session, but STATE_UPDATE's "role" field is not
// - each recipient must get its own JSON with its own role, never one shared
// string sent to both connections. players is likewise passed in already
// assembled: this mapper stays a pure (snapshot, color, players) -> json
// function and does not itself reach into ConnectionManager/IIdentityStore
// - that assembly is MakeMoveUseCase's job, the one place that already
// talks to both.
namespace GameSnapshotMapper {

/// Converts a domain GameSnapshot plus one recipient's color plus the full player roster into that recipient's STATE_UPDATE payload JSON.
nlohmann::json toJson(const GameSnapshot& snapshot, char recipientColor, const std::vector<PlayerDto>& players);

/// Converts a parsed STATE_UPDATE DTO back into a domain GameSnapshot - the reverse of toJson, needed client-side (ServerConnection). dto.role/dto.players are deliberately NOT part of GameSnapshot - core/ has no session concept - callers read those directly off the DTO instead.
GameSnapshot fromDto(const StateUpdateDto& dto);

}  // namespace GameSnapshotMapper
