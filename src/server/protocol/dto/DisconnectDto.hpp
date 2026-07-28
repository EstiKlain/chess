#pragma once

#include <nlohmann/json.hpp>

// DISCONNECT_COUNTDOWN payload: { secondsLeft }, server->client only - this
// server never parses one, only sends it (a client-side from_json can be
// added when chess_gui actually consumes it, out of this iteration's scope,
// mirroring how StateUpdateDto carries both directions for the same reason).
struct DisconnectDto {
    int secondsLeft = 0;
};

/// Serializes a DisconnectDto to its wire JSON representation.
inline void to_json(nlohmann::json& j, const DisconnectDto& d) { j = nlohmann::json{{"secondsLeft", d.secondsLeft}}; }
