#include "doctest.h"

#include <sstream>
#include <string>

#include "server/infrastructure/logging/FileLogger.hpp"

TEST_CASE("FileLogger: writes one formatted line per call, with a deterministic injected timestamp") {
    std::ostringstream out;
    FileLogger logger(out, [] { return "2026-07-26T10:15:32.123"; });

    logger.log("SENT", "conn-1", R"({"type":"MOVE"})");

    CHECK(out.str() == "[2026-07-26T10:15:32.123] SENT conn-1 {\"type\":\"MOVE\"}\n");
}

TEST_CASE("FileLogger: multiple calls append in order, each on its own line") {
    std::ostringstream out;
    FileLogger logger(out, [] { return "t"; });

    logger.log("SENT", "conn-1", "first");
    logger.log("RECEIVED", "conn-2", "second");

    CHECK(out.str() == "[t] SENT conn-1 first\n[t] RECEIVED conn-2 second\n");
}

TEST_CASE("FileLogger: empty connectionId is still written as an empty field, keeping the 4-column shape") {
    std::ostringstream out;
    FileLogger logger(out, [] { return "t"; });

    logger.log("SENT", "", "payload");

    CHECK(out.str() == "[t] SENT  payload\n");
}

TEST_CASE("FileLogger: with no timestamp provider given, falls back to a real (non-empty) timestamp") {
    std::ostringstream out;
    FileLogger logger(out);

    logger.log("SENT", "conn-1", "payload");

    CHECK(out.str().find("SENT conn-1 payload") != std::string::npos);
    CHECK(out.str().rfind('[', 0) == 0);
}
