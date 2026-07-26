#pragma once

#include <functional>
#include <ostream>
#include <string>

#include "server/domain_ports/ILogger.hpp"

class FileLogger : public ILogger {
public:
    using TimestampProvider = std::function<std::string()>;

    /// Constructs a logger writing to out. If nowProvider is empty, uses the real wall-clock (ISO-8601 with milliseconds).
    explicit FileLogger(std::ostream& out, TimestampProvider nowProvider = nullptr);

    /// Writes one line: "[<timestamp>] <direction> <connectionId> <rawJson>", then flushes.
    void log(const std::string& direction, const std::string& connectionId, const std::string& rawJson) override;

private:
    std::ostream& out_;
    TimestampProvider nowProvider_;
};
