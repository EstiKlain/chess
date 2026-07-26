#pragma once

#include <string>

class ILogger {
public:
    virtual ~ILogger() = default;

    /// Records one message crossing the wire. direction is "SENT" or "RECEIVED". connectionId identifies which network connection this message belongs to on the server side; left empty on the client side, where there is only ever one connection. rawJson is the exact envelope bytes, logged verbatim.
    virtual void log(const std::string& direction, const std::string& connectionId, const std::string& rawJson) = 0;
};
