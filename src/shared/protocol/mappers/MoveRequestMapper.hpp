#pragma once

#include "engine/MoveRequest.hpp"
#include "shared/protocol/dto/MoveDto.hpp"

// The one place that knows about both core::MoveRequest and MoveDto - per
// the plan, mappers are the single meeting point between core/ and
// protocol/, so nothing else in server/ needs to include both.
namespace MoveRequestMapper {

/// Converts a wire-level MoveDto into the domain-level MoveRequest GameEngine::requestMove expects. requesterColor is never read from the client-supplied dto (never trust the client to self-report its color) - callers must pass the color from the server-side ConnectionBinding instead.
MoveRequest toDomain(const MoveDto& dto, char requesterColor = '\0');

/// Converts a domain-level MoveRequest back into a wire-level MoveDto (used by the mapper's own round-trip test).
MoveDto toDto(const MoveRequest& request);

}  // namespace MoveRequestMapper
