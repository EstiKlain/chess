#pragma once

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#ifndef _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_STL_
#endif

#include <cstdint>
#include <string>
#include <thread>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "client_net/IServerLink.hpp"

// Concrete IServerLink: websocketpp + standalone asio, the client-side
// mirror of WebSocketTransport (src/server/infrastructure/transport/). This
// class, together with ServerConnection's own message parsing, is the only
// code on the client that knows a raw JSON string is travelling over a
// WebSocket at all.
class WebSocketClientLink : public IServerLink {
public:
    WebSocketClientLink();
    ~WebSocketClientLink() override;

    void connect(const std::string& host, uint16_t port) override;
    void send(const std::string& rawJson) override;
    void setOnMessage(OnMessageHandler handler) override;
    void stop() override;

private:
    using Client = websocketpp::client<websocketpp::config::asio_client>;
    using ConnectionHandle = websocketpp::connection_hdl;

    Client client_;
    ConnectionHandle handle_;
    std::thread ioThread_;
    OnMessageHandler onMessage_;
};
