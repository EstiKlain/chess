#pragma once

#include <cstdint>
#include <string>

#include "client_net/domain_ports/IServerLink.hpp"
#include "shared/logging/ILogger.hpp"

// Decorator: implements IServerLink itself, wrapping the real link
// (concretely WebSocketClientLink, but never named here - only
// IServerLink&). Wired in between WebSocketClientLink and ServerConnection
// in main_gui.cpp, so ServerConnection needs no change - it was always
// written against IServerLink&, never against WebSocketClientLink by name.
// The exact client-side mirror of the server's LoggingTransport. Reuses the
// server's ILogger port directly rather than duplicating it - chess_gui
// already links server/protocol/ files today (Iteration 3.5's "one source
// of truth, don't duplicate" decision for DTOs), same reasoning applies to
// the logging port.
class ClientLogger : public IServerLink {
public:
    ClientLogger(IServerLink& real, ILogger& logger);

    /// Pure passthrough - connection lifecycle isn't a "message," not logged.
    void connect(const std::string& host, uint16_t port) override;

    /// Logs SENT with an empty connectionId (a single connection has no id to carry), then delegates.
    void send(const std::string& rawJson) override;

    /// Registers a wrapping handler with the real link that logs RECEIVED, then invokes the given handler unchanged.
    void setOnMessage(OnMessageHandler handler) override;

    /// Registers a wrapping handler with the real link that logs CLOSED, then invokes the given handler unchanged.
    void setOnClose(OnCloseHandler handler) override;

    /// Pure passthrough to the real link's stop().
    void stop() override;

private:
    IServerLink& real_;
    ILogger& logger_;
};
