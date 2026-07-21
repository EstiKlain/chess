#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "server/domain_ports/ITransport.hpp"

// Concrete ITransport built on websocketpp + standalone asio.
// This class, together with MessageEnvelope.hpp, is the ONLY code in the
// server that knows a raw JSON string is actually travelling over a
// WebSocket. MessageRouter, IEventBus and ConnectionManager never see a
// socket, a connection_hdl, or websocketpp at all.
class WebSocketTransport : public ITransport {
public:
    using Server = websocketpp::server<websocketpp::config::asio>;
    using ConnectionHandle = websocketpp::connection_hdl;

    WebSocketTransport();

    void send(const std::string& connectionId, const std::string& rawJson) override;
    void broadcast(const std::string& rawJson) override;

    void setOnOpen(OnOpenHandler handler) override;
    void setOnClose(OnCloseHandler handler) override;
    void setOnMessage(OnMessageHandler handler) override;

    void run(uint16_t port) override;
    void stop() override;

private:
    // connection_hdl has no natural string form; we derive a stable id from
    // the handle's underlying pointer address. Good enough for iteration 1 -
    // revisit only if a future iteration needs ids that survive a reconnect
    // (iteration 5 solves that separately, with sessionToken, not connectionId).
    std::string idFor(const ConnectionHandle& hdl) const;

    Server server_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ConnectionHandle> connections_;

    OnOpenHandler onOpen_;
    OnCloseHandler onClose_;
    OnMessageHandler onMessage_;
};
