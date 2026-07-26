#include "server/infrastructure/logging/FileLogger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

constexpr int kMillisecondsPerSecond = 1000;
constexpr int kMillisecondFieldWidth = 3;

std::string realNowIso8601() {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % kMillisecondsPerSecond;
    const std::time_t nowTimeT = system_clock::to_time_t(now);

    std::tm nowTm{};
#if defined(_WIN32)
    gmtime_s(&nowTm, &nowTimeT);
#else
    gmtime_r(&nowTimeT, &nowTm);
#endif

    std::ostringstream out;
    out << std::put_time(&nowTm, "%Y-%m-%dT%H:%M:%S");
    out << '.' << std::setfill('0') << std::setw(kMillisecondFieldWidth) << ms.count();
    return out.str();
}

}  // namespace

FileLogger::FileLogger(std::ostream& out, TimestampProvider nowProvider)
    : out_(out), nowProvider_(nowProvider ? std::move(nowProvider) : &realNowIso8601) {}

void FileLogger::log(const std::string& direction, const std::string& connectionId, const std::string& rawJson) {
    out_ << '[' << nowProvider_() << "] " << direction << ' ' << connectionId << ' ' << rawJson << '\n';
    out_.flush();
}
