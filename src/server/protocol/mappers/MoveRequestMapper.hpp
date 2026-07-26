#pragma once

#include "engine/MoveRequest.hpp"
#include "server/protocol/dto/MoveDto.hpp"

// The one place that knows about both core::MoveRequest and MoveDto - per
// the plan, mappers are the single meeting point between core/ and
// protocol/, so nothing else in server/ needs to include both.
namespace MoveRequestMapper {

/// Converts a wire-level MoveDto into the domain-level MoveRequest GameEngine::requestMove expects.
MoveRequest toDomain(const MoveDto& dto);

/// Converts a domain-level MoveRequest back into a wire-level MoveDto (used by the mapper's own round-trip test).
MoveDto toDto(const MoveRequest& request);

}  // namespace MoveRequestMapper
