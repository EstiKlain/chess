#pragma once

#include <cstdint>
#include <functional>
#include <string>

// Port: the client-side mirror of the server's ITransport, shaped for
// exactly one outbound connection (no connectionId, no broadcast()) -
// "connect, send/receive raw text" only. ServerConnection depends on THIS,
// never on WebSocketClientLink directly - lets ServerConnection's login/
// snapshot-parsing logic be unit tested with a fake, never a real socket.
class IServerLink {
public:
    virtual ~IServerLink() = default;

    /// Opens the connection to host:port and blocks until the handshake completes. Throws std::runtime_error on failure.
    virtual void connect(const std::string& host, uint16_t port) = 0;

    /// Sends a raw JSON string.
    virtual void send(const std::string& rawJson) = 0;

    using OnMessageHandler = std::function<void(const std::string& rawJson)>;

    /// Registered once, before connect(). Called for every message from the server, on whatever thread the link delivers messages on.
    virtual void setOnMessage(OnMessageHandler handler) = 0;

    /// Stops the connection and any background thread.
    virtual void stop() = 0;
};
