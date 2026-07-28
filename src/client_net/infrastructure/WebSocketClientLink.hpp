#pragma once

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#ifndef _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_STL_
#endif

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "client_net/domain_ports/IServerLink.hpp"

// Concrete IServerLink: websocketpp + standalone asio, the client-side
// mirror of WebSocketTransport (src/server/infrastructure/transport/). This
// class, together with ServerConnection's own message parsing, is the only
// code on the client that knows a raw JSON string is travelling over a
// WebSocket at all.
//
// Reconnectable in place (Server-Iteration 5 follow-up): the endpoint runs
// in "perpetual" mode (client_.start_perpetual(), started once alongside
// ioThread_ in the constructor) so the io thread survives a connection
// closing and stays available for a later connect() call to redial on -
// verified against a real server with an isolated spike before this was
// built. connect() itself never touches ioThread_ (that would race/crash
// once perpetual mode is on - see the .cpp for the history of why).
class WebSocketClientLink : public IServerLink {
public:
    WebSocketClientLink();
    ~WebSocketClientLink() override;

    void connect(const std::string& host, uint16_t port) override;
    void send(const std::string& rawJson) override;
    void setOnMessage(OnMessageHandler handler) override;
    void setOnClose(OnCloseHandler handler) override;
    void stop() override;

private:
    using Client = websocketpp::client<websocketpp::config::asio_client>;
    using ConnectionHandle = websocketpp::connection_hdl;

    Client client_;
    ConnectionHandle handle_;
    std::thread ioThread_;
    OnMessageHandler onMessage_;
    OnCloseHandler onClose_;
    std::atomic<bool> stopping_{false};
};
