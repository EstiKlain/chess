#include "server/application/MakeMoveUseCase.hpp"

#include <exception>

#include "server/application/GameSession.hpp"
#include "server/application/StateFanOut.hpp"
#include "server/protocol/Envelope.hpp"
#include "shared/protocol/dto/JumpDto.hpp"
#include "shared/protocol/dto/MoveDto.hpp"
#include "shared/protocol/mappers/MoveRequestMapper.hpp"

MakeMoveUseCase::MakeMoveUseCase(IEventBus &bus, ITransport &transport, ConnectionManager &connections,
                                 IIdentityStore &identities)
    : bus_(bus), transport_(transport), connections_(connections), identities_(identities) {}

void MakeMoveUseCase::handleMove(const std::string &connectionId, const std::string &requestId,
                                 const nlohmann::json &payload)
{
    const auto bindingOpt = lookupBoundSession(connectionId, requestId);
    if (!bindingOpt.has_value())
    {
        return;
    }

    MoveDto dto;
    try
    {
        dto = payload.get<MoveDto>();
    }
    catch (const std::exception &ex)
    {
        protocol::sendError(transport_, connectionId, requestId, "MALFORMED_PAYLOAD",
                            std::string("malformed MOVE payload: ") + ex.what());
        return;
    }

    GameSession &session = *bindingOpt->session;
    const MoveResult result = session.requestMove(MoveRequestMapper::toDomain(dto, bindingOpt->color));
    finishRequest(connectionId, requestId, session, result.accepted, result.reason);
}

void MakeMoveUseCase::handleJump(const std::string &connectionId, const std::string &requestId,
                                 const nlohmann::json &payload)
{
    const auto bindingOpt = lookupBoundSession(connectionId, requestId);
    if (!bindingOpt.has_value())
    {
        return;
    }

    JumpDto dto;
    try
    {
        dto = payload.get<JumpDto>();
    }
    catch (const std::exception &ex)
    {
        protocol::sendError(transport_, connectionId, requestId, "MALFORMED_PAYLOAD",
                            std::string("malformed JUMP payload: ") + ex.what());
        return;
    }

    GameSession &session = *bindingOpt->session;
    const JumpResult result = session.requestJump(dto.row, dto.col, bindingOpt->color);
    finishRequest(connectionId, requestId, session, result.accepted, result.reason);
}

std::optional<ConnectionBinding> MakeMoveUseCase::lookupBoundSession(const std::string &connectionId,
                                                                     const std::string &requestId)
{
    const auto binding = connections_.sessionFor(connectionId);
    if (!binding.has_value())
    {
        protocol::sendError(transport_, connectionId, requestId, "NOT_IN_GAME", "not currently in a game");
        return std::nullopt;
    }
    if (binding->session == nullptr)
    {
        protocol::sendError(transport_, connectionId, requestId, "INTERNAL_ERROR",
                            "connection is bound but its session pointer is null");
        return std::nullopt;
    }
    return binding;
}

void MakeMoveUseCase::finishRequest(const std::string &connectionId, const std::string &requestId,
                                    GameSession &session, bool accepted, const std::string &reason)
{
    if (!accepted)
    {
        protocol::sendError(transport_, connectionId, requestId, "ILLEGAL_MOVE", reason);
        return;
    }
    fanOutStateUpdate(connectionId, requestId, session);
}

void MakeMoveUseCase::fanOutStateUpdate(const std::string &connectionId, const std::string &requestId,
                                        GameSession &session)
{
    // Hook for future subscribers (logger in Iteration 4, sound/animation in
    // Iteration 9) - nobody consumes it yet, but the event exists from the
    // moment moves become real, per the plan. Not correlated to any single
    // client request, so requestId is left empty here.
    bus_.publish(BusEvent{"MoveApplied", connectionId, "", nlohmann::json::object()});

    StateFanOut::broadcast(session, connections_, identities_, transport_, connectionId, requestId);
}
