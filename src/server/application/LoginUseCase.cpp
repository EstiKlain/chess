#include "server/application/LoginUseCase.hpp"

#include <exception>

#include "server/protocol/Envelope.hpp"
#include "server/protocol/dto/LoginDto.hpp"

LoginUseCase::LoginUseCase(IIdentityStore& identities, ITransport& transport, ConnectionManager& connections)
    : identities_(identities), transport_(transport), connections_(connections) {}

void LoginUseCase::handleLogin(const std::string& connectionId, const std::string& requestId,
                                const nlohmann::json& payload) {
    if (!connections_.sessionFor(connectionId).has_value()) {
        protocol::sendError(transport_, connectionId, requestId, "TABLE_FULL", "this table already has two players");
        return;
    }

    LoginDto dto;
    try {
        dto = payload.get<LoginDto>();
    } catch (const std::exception& ex) {
        protocol::sendError(transport_, connectionId, requestId, "MALFORMED_PAYLOAD",
                             std::string("malformed LOGIN payload: ") + ex.what());
        return;
    }

    // Same MALFORMED_PAYLOAD code as a missing field above - an empty
    // username is the same category of failure (bad shape of client-
    // supplied input), not a new kind of rejection needing its own code.
    if (dto.username.empty()) {
        protocol::sendError(transport_, connectionId, requestId, "MALFORMED_PAYLOAD", "username must not be empty");
        return;
    }

    identities_.login(connectionId, dto.username);
    transport_.send(connectionId, protocol::envelope("LOGIN_OK", requestId, nlohmann::json::object()));
}
