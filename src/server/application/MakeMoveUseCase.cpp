#include "server/application/MakeMoveUseCase.hpp"

#include <exception>

#include "server/application/GameSession.hpp"
#include "server/protocol/Envelope.hpp"
#include "server/protocol/dto/JumpDto.hpp"
#include "server/protocol/dto/MoveDto.hpp"
#include "server/protocol/mappers/GameSnapshotMapper.hpp"
#include "server/protocol/mappers/MoveRequestMapper.hpp"

MakeMoveUseCase::MakeMoveUseCase(IEventBus& bus, ITransport& transport, ConnectionManager& connections)
    : bus_(bus), transport_(transport), connections_(connections) {}

void MakeMoveUseCase::handleMove(const std::string& connectionId, const nlohmann::json& payload) {
    const auto binding = connections_.sessionFor(connectionId);
    if (!binding.has_value() || binding->session == nullptr) {
        sendError(connectionId, "INTERNAL_ERROR", "connection is not bound to a game session");
        return;
    }

    MoveDto dto;
    try {
        dto = payload.get<MoveDto>();
    } catch (const std::exception& ex) {
        sendError(connectionId, "MALFORMED_PAYLOAD", std::string("malformed MOVE payload: ") + ex.what());
        return;
    }

    GameSession& session = *binding->session;
    const MoveResult result = session.requestMove(MoveRequestMapper::toDomain(dto));

    if (!result.accepted) {
        sendError(connectionId, "ILLEGAL_MOVE", result.reason);
        return;
    }

    fanOutStateUpdate(connectionId, session);
}

void MakeMoveUseCase::handleJump(const std::string& connectionId, const nlohmann::json& payload) {
    const auto binding = connections_.sessionFor(connectionId);
    if (!binding.has_value() || binding->session == nullptr) {
        sendError(connectionId, "INTERNAL_ERROR", "connection is not bound to a game session");
        return;
    }

    JumpDto dto;
    try {
        dto = payload.get<JumpDto>();
    } catch (const std::exception& ex) {
        sendError(connectionId, "MALFORMED_PAYLOAD", std::string("malformed JUMP payload: ") + ex.what());
        return;
    }

    GameSession& session = *binding->session;
    const JumpResult result = session.requestJump(dto.row, dto.col);

    if (!result.accepted) {
        sendError(connectionId, "ILLEGAL_MOVE", result.reason);
        return;
    }

    fanOutStateUpdate(connectionId, session);
}

void MakeMoveUseCase::fanOutStateUpdate(const std::string& connectionId, GameSession& session) {
    // Hook for future subscribers (logger in Iteration 4, sound/animation in
    // Iteration 9) - nobody consumes it yet, but the event exists from the
    // moment moves become real, per the plan.
    bus_.publish(BusEvent{"MoveApplied", connectionId, nlohmann::json::object()});

    const GameSnapshot snapshot = session.snapshot();
    for (const std::string& recipientId : connections_.connectionIdsFor(&session)) {
        const auto recipientBinding = connections_.sessionFor(recipientId);
        if (!recipientBinding.has_value()) {
            continue;
        }
        transport_.send(recipientId,
                         protocol::envelope("STATE_UPDATE", GameSnapshotMapper::toJson(snapshot, recipientBinding->color)));
    }
}

void MakeMoveUseCase::sendError(const std::string& connectionId, const std::string& code,
                                 const std::string& message) {
    transport_.send(connectionId, protocol::errorEnvelope(code, message));
}
