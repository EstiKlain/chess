#include "doctest.h"

#include "shared/protocol/LogRedaction.hpp"

TEST_CASE("LogRedaction: a LOGIN message's password is replaced with ***") {
    const std::string raw = R"({"type":"LOGIN","requestId":"r1","payload":{"username":"alice","password":"hunter2"}})";

    const std::string redacted = protocol::redactForLog(raw);

    CHECK(redacted.find("hunter2") == std::string::npos);
    CHECK(redacted.find("\"password\":\"***\"") != std::string::npos);
    CHECK(redacted.find("\"username\":\"alice\"") != std::string::npos);
    CHECK(redacted.find("\"type\":\"LOGIN\"") != std::string::npos);
}

TEST_CASE("LogRedaction: a non-LOGIN message is returned byte-for-byte unchanged") {
    const std::string raw = R"({"type":"STATE_UPDATE","requestId":"","payload":{"rows":8,"cols":8}})";

    CHECK(protocol::redactForLog(raw) == raw);
}

TEST_CASE("LogRedaction: a LOGIN message with no password field is returned unchanged") {
    const std::string raw = R"({"type":"LOGIN","requestId":"r1","payload":{"username":"alice"}})";

    CHECK(protocol::redactForLog(raw) == raw);
}

TEST_CASE("LogRedaction: malformed JSON is returned unchanged rather than throwing") {
    const std::string raw = "not valid json { \"password\":";

    CHECK(protocol::redactForLog(raw) == raw);
}

TEST_CASE("LogRedaction: a message that merely mentions the word password outside LOGIN is unchanged") {
    const std::string raw = R"({"type":"ERROR","requestId":"r1","payload":{"code":"MALFORMED_PAYLOAD","message":"password must not be empty"}})";

    CHECK(protocol::redactForLog(raw) == raw);
}
