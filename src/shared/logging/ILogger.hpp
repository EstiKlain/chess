#pragma once

#include <string>

/// Which way a logged message crossed the wire.
enum class LogDirection {
    Sent,
    Received,
};

class ILogger {
public:
    virtual ~ILogger() = default;

    /// Records one message crossing the wire. connectionId identifies which network connection this message belongs to on the server side; left empty on the client side, where there is only ever one connection. rawJson is the exact envelope bytes, logged verbatim.
    virtual void log(LogDirection direction, const std::string& connectionId, const std::string& rawJson) = 0;
};
