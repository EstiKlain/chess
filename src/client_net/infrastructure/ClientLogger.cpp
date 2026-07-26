#include "client_net/infrastructure/ClientLogger.hpp"

#include <utility>

ClientLogger::ClientLogger(IServerLink& real, ILogger& logger) : real_(real), logger_(logger) {}

void ClientLogger::connect(const std::string& host, uint16_t port) {
    real_.connect(host, port);
}

void ClientLogger::send(const std::string& rawJson) {
    logger_.log("SENT", "", rawJson);
    real_.send(rawJson);
}

void ClientLogger::setOnMessage(OnMessageHandler handler) {
    real_.setOnMessage([this, handler = std::move(handler)](const std::string& rawJson) {
        logger_.log("RECEIVED", "", rawJson);
        handler(rawJson);
    });
}

void ClientLogger::stop() {
    real_.stop();
}
