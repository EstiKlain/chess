#include "server/protocol/MessageRouter.hpp"

#include <exception>

#include "server/protocol/dto/MessageEnvelope.hpp"

MessageRouter::MessageRouter(IEventBus& bus, ITransport& transport)
    : bus_(bus), transport_(transport) {}

void MessageRouter::handleRawMessage(const std::string& connectionId,
                                      const std::string& rawJson) {
    MessageEnvelope envelope;

    try {
        envelope = nlohmann::json::parse(rawJson).get<MessageEnvelope>();
    } catch (const std::exception& ex) {
        nlohmann::json error = {
            {"type", "ERROR"},
            {"requestId", ""},
            {"payload",
             {
                 {"code", "INTERNAL_ERROR"},
                 {"message", std::string("malformed message: ") + ex.what()},
             }},
        };
        transport_.send(connectionId, error.dump());
        return;
    }

    bus_.publish(BusEvent{envelope.type, connectionId, envelope.payload});
}
