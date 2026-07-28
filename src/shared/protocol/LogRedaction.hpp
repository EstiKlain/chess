#pragma once

#include <string>

namespace protocol {

/// Returns rawJson unchanged unless it's a LOGIN envelope carrying a "password" field in its payload, in which case that field's value is replaced with "***" in the returned copy. Only affects what gets written to a log file - never touches the real message being processed (LoggingTransport/ClientLogger call this only on the string handed to ILogger::log, not on the string forwarded to the real handler).
/// @param rawJson The raw wire-format JSON string, exactly as sent/received.
/// @return rawJson, or a redacted copy if it was a LOGIN message with a password.
std::string redactForLog(const std::string& rawJson);

}  // namespace protocol
