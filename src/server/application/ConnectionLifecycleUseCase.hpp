#pragma once

#include <string>

#include "server/application/ConnectionManager.hpp"
#include "server/application/DisconnectUseCase.hpp"
#include "server/application/PlayRequestUseCase.hpp"
#include "server/domain_ports/IIdentityStore.hpp"

// Everything that must happen when a connection closes. Used to be four
// inline calls in main_server.cpp's setOnClose lambda; became a class once
// PlayRequestUseCase added a fourth ordered step (queue cleanup).
class ConnectionLifecycleUseCase {
public:
    ConnectionLifecycleUseCase(ConnectionManager& connections, IIdentityStore& identities,
                                DisconnectUseCase& disconnectUseCase, PlayRequestUseCase& playRequestUseCase);

    /// Wired from transport.setOnClose.
    void onDisconnected(const std::string& connectionId);

private:
    ConnectionManager& connections_;
    IIdentityStore& identities_;
    DisconnectUseCase& disconnectUseCase_;
    PlayRequestUseCase& playRequestUseCase_;
};
