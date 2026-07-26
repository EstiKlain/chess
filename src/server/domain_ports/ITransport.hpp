#pragma once

#include <cstdint>
#include <functional>
#include <string>

// Port: an abstract "send/receive raw text over some connection" contract.
// MessageRouter and main_server.cpp depend on THIS, never on
// WebSocketTransport directly - that's what lets MessageRouter be unit
// tested with a fake transport, with no real socket involved.
class ITransport {
public:
    virtual ~ITransport() = default;

    // Sends a raw JSON string to one specific, already-connected client.
    // If connectionId is unknown (already disconnected), this is a no-op.
    virtual void send(const std::string& connectionId, const std::string& rawJson) = 0;

    // Sends a raw JSON string to every currently open connection.
    // Not used by iteration 1's PING/PONG flow, but declared now so
    // iteration 2's STATE_UPDATE-to-both-players doesn't need an
    // interface change.
    virtual void broadcast(const std::string& rawJson) = 0;

    using OnOpenHandler = std::function<void(const std::string& connectionId)>;
    using OnCloseHandler = std::function<void(const std::string& connectionId)>;
    using OnMessageHandler =std::function<void(const std::string& connectionId, const std::string& rawJson)>;

    // Registered once, by whoever wires the system together (main_server.cpp),
    // before run() is called.
    virtual void setOnOpen(OnOpenHandler handler) = 0;
    virtual void setOnClose(OnCloseHandler handler) = 0;
    virtual void setOnMessage(OnMessageHandler handler) = 0;

    // Starts the network event loop. Blocks the calling thread until stop()
    // is called (typically from another thread, or a signal handler - not
    // needed yet in iteration 1).
    virtual void run(uint16_t port) = 0;
    virtual void stop() = 0;
};
