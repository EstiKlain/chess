#pragma once

#include <nlohmann/json.hpp>

#include "engine/GameSnapshot.hpp"

// The other core/<->protocol/ meeting point (alongside MoveRequestMapper).
// Takes the recipient's color explicitly: GameSnapshot itself is shared
// between both players in a session, but STATE_UPDATE's "role" field is not
// - each recipient must get its own JSON with its own role, never one shared
// string sent to both connections.
namespace GameSnapshotMapper {

/// Converts a domain GameSnapshot plus one recipient's color into that recipient's STATE_UPDATE payload JSON.
nlohmann::json toJson(const GameSnapshot& snapshot, char recipientColor);

}  // namespace GameSnapshotMapper
