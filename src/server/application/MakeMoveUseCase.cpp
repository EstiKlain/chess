#include "server/application/MakeMoveUseCase.hpp"

#include <exception>

#include "server/application/GameSession.hpp"
#include "server/application/StateFanOut.hpp"
#include "server/protocol/Envelope.hpp"
#include "server/protocol/dto/JumpDto.hpp"
#include "server/protocol/dto/MoveDto.hpp"
#include "server/protocol/mappers/MoveRequestMapper.hpp"

MakeMoveUseCase::MakeMoveUseCase(IEventBus &bus, ITransport &transport, ConnectionManager &connections,
                                 IIdentityStore &identities)
    : bus_(bus), transport_(transport), connections_(connections), identities_(identities) {}

void MakeMoveUseCase::handleMove(const std::string &connectionId, const std::string &requestId,
                                 const nlohmann::json &payload)
{
    const auto sessionOpt = lookupBoundSession(connectionId, requestId);
    if (!sessionOpt.has_value())
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

    GameSession &session = **sessionOpt;
    const MoveResult result = session.requestMove(MoveRequestMapper::toDomain(dto));
    finishRequest(connectionId, requestId, session, result.accepted, result.reason);
}

void MakeMoveUseCase::handleJump(const std::string &connectionId, const std::string &requestId,
                                 const nlohmann::json &payload)
{
    const auto sessionOpt = lookupBoundSession(connectionId, requestId);
    if (!sessionOpt.has_value())
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

    GameSession &session = **sessionOpt;
    const JumpResult result = session.requestJump(dto.row, dto.col);
    finishRequest(connectionId, requestId, session, result.accepted, result.reason);
}

std::optional<GameSession *> MakeMoveUseCase::lookupBoundSession(const std::string &connectionId,
                                                                 const std::string &requestId)
{
    const auto binding = connections_.sessionFor(connectionId);
    if (!binding.has_value() || binding->session == nullptr)
    {
        protocol::sendError(transport_, connectionId, requestId, "INTERNAL_ERROR",
                            "connection is not bound to a game session");
        return std::nullopt;
    }
    return binding->session;
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
