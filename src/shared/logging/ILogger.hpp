#pragma once

#include <string>

/// Which way a logged message crossed the wire. Closed is not a message
/// direction in the same sense as Sent/Received - it records that the
/// connection itself died (Server-Iteration 5's client-side auto-reconnect
/// follow-up), reusing this same log stream rather than inventing a
/// separate one for a single event type.
enum class LogDirection {
    Sent,
    Received,
    Closed,
};

class ILogger {
public:
    virtual ~ILogger() = default;

    /// Records one message crossing the wire. connectionId identifies which network connection this message belongs to on the server side; left empty on the client side, where there is only ever one connection. rawJson is the exact envelope bytes, logged verbatim.
    virtual void log(LogDirection direction, const std::string& connectionId, const std::string& rawJson) = 0;
};
