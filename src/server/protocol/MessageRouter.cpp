#include "server/protocol/MessageRouter.hpp"

#include <exception>

#include "server/protocol/Envelope.hpp"
#include "server/protocol/dto/MessageEnvelope.hpp"

MessageRouter::MessageRouter(IEventBus& bus, ITransport& transport)
    : bus_(bus), transport_(transport) {}

void MessageRouter::handleRawMessage(const std::string& connectionId,
                                      const std::string& rawJson) {
    MessageEnvelope envelope;

    try {
        envelope = nlohmann::json::parse(rawJson).get<MessageEnvelope>();
    } catch (const std::exception& ex) {
        transport_.send(connectionId,
                         protocol::errorEnvelope("INTERNAL_ERROR",
                                                  std::string("malformed message: ") + ex.what()));
        return;
    }

    bus_.publish(BusEvent{envelope.type, connectionId, envelope.requestId, envelope.payload});
}
