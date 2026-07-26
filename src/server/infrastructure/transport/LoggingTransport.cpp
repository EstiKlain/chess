#include "server/infrastructure/transport/LoggingTransport.hpp"

#include <utility>

LoggingTransport::LoggingTransport(ITransport& real, ILogger& logger) : real_(real), logger_(logger) {}

void LoggingTransport::send(const std::string& connectionId, const std::string& rawJson) {
    logger_.log("SENT", connectionId, rawJson);
    real_.send(connectionId, rawJson);
}

void LoggingTransport::broadcast(const std::string& rawJson) {
    logger_.log("SENT", "", rawJson);
    real_.broadcast(rawJson);
}

void LoggingTransport::setOnOpen(OnOpenHandler handler) {
    real_.setOnOpen(std::move(handler));
}

void LoggingTransport::setOnClose(OnCloseHandler handler) {
    real_.setOnClose(std::move(handler));
}

void LoggingTransport::setOnMessage(OnMessageHandler handler) {
    real_.setOnMessage([this, handler = std::move(handler)](const std::string& connectionId, const std::string& rawJson) {
        logger_.log("RECEIVED", connectionId, rawJson);
        handler(connectionId, rawJson);
    });
}

void LoggingTransport::run(uint16_t port) {
    real_.run(port);
}

void LoggingTransport::stop() {
    real_.stop();
}
