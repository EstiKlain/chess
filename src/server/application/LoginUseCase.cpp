#include "server/application/LoginUseCase.hpp"

#include <exception>

#include "server/config.hpp"
#include "server/protocol/Envelope.hpp"
#include "server/protocol/dto/LoginDto.hpp"

LoginUseCase::LoginUseCase(IIdentityStore& identities, ITransport& transport, ConnectionManager& connections,
                            PlayerSessionRegistry& sessions, IUserRepository& users, IPasswordHasher& hasher)
    : identities_(identities),
      transport_(transport),
      connections_(connections),
      sessions_(sessions),
      users_(users),
      hasher_(hasher) {}

void LoginUseCase::handleLogin(const std::string& connectionId, const std::string& requestId,
                                const nlohmann::json& payload) {
    const auto binding = connections_.sessionFor(connectionId);
    if (!binding.has_value()) {
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
    // username/password is the same category of failure (bad shape of
    // client-supplied input), not a new kind of rejection needing its own
    // code.
    if (dto.username.empty()) {
        protocol::sendError(transport_, connectionId, requestId, "MALFORMED_PAYLOAD", "username must not be empty");
        return;
    }
    if (dto.password.empty()) {
        protocol::sendError(transport_, connectionId, requestId, "MALFORMED_PAYLOAD", "password must not be empty");
        return;
    }

    int elo = server_config::kInitialElo;
    const auto record = users_.findByUsername(dto.username);
    if (!record.has_value()) {
        users_.createUser(dto.username, hasher_.hash(dto.password));
        // elo stays kInitialElo - matches the value createUser stores, not re-read here.
    } else {
        if (!hasher_.verify(dto.password, record->passwordHash)) {
            protocol::sendError(transport_, connectionId, requestId, "INVALID_CREDENTIALS", "wrong password");
            return;
        }
        elo = record->elo;
    }

    identities_.login(connectionId, dto.username);
    connections_.setElo(connectionId, elo);
    const std::string token = sessions_.registerNew(dto.username, binding->color, binding->session, connectionId);
    transport_.send(connectionId,
                     protocol::envelope("LOGIN_OK", requestId, nlohmann::json{{"sessionToken", token}}));
}
