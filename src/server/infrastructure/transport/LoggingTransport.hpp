#pragma once

#include <cstdint>
#include <string>

#include "shared/logging/ILogger.hpp"
#include "server/domain_ports/ITransport.hpp"

// Decorator: implements ITransport itself, wrapping the real transport
// (concretely WebSocketTransport, but never named here - only ITransport&).
// Wired in between WebSocketTransport and everything downstream (AuthGuard,
// MessageRouter, MakeMoveUseCase, LoginUseCase, StateFanOut, the tick
// thread) in main_server.cpp, so none of them need any change - they were
// always written against ITransport&, never against WebSocketTransport by
// name. Mirrors AuthGuard's own decorator-around-a-port precedent exactly,
// one layer over (ITransport instead of IEventBus). This is the only way
// to log every SENT message: outgoing sends never touch the IEventBus, so
// a bus subscriber could only ever see RECEIVED traffic.
class LoggingTransport : public ITransport {
public:
    LoggingTransport(ITransport& real, ILogger& logger);

    /// Logs SENT with connectionId, then delegates to the real transport.
    void send(const std::string& connectionId, const std::string& rawJson) override;

    /// Logs SENT with an empty connectionId (no single recipient), then delegates to the real transport.
    void broadcast(const std::string& rawJson) override;

    /// Pure passthrough - connection open isn't a "message," not logged.
    void setOnOpen(OnOpenHandler handler) override;

    /// Pure passthrough - connection close isn't a "message," not logged.
    void setOnClose(OnCloseHandler handler) override;

    /// Registers a wrapping handler with the real transport that logs RECEIVED with connectionId, then invokes the given handler unchanged.
    void setOnMessage(OnMessageHandler handler) override;

    /// Pure passthrough to the real transport's run().
    void run(uint16_t port) override;

    /// Pure passthrough to the real transport's stop().
    void stop() override;

private:
    ITransport& real_;
    ILogger& logger_;
};
