#include "server/application/ConnectionLifecycleUseCase.hpp"

ConnectionLifecycleUseCase::ConnectionLifecycleUseCase(ConnectionManager& connections, IIdentityStore& identities,
                                                         DisconnectUseCase& disconnectUseCase,
                                                         PlayRequestUseCase& playRequestUseCase)
    : connections_(connections),
      identities_(identities),
      disconnectUseCase_(disconnectUseCase),
      playRequestUseCase_(playRequestUseCase) {}

void ConnectionLifecycleUseCase::onDisconnected(const std::string& connectionId) {
    connections_.onDisconnected(connectionId);
    identities_.logout(connectionId);
    disconnectUseCase_.onDisconnected(connectionId);
    playRequestUseCase_.onDisconnected(connectionId);
}
