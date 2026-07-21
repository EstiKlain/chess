#pragma once

#include <string>

#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/ITransport.hpp"

// Parses a raw JSON string into a MessageEnvelope and decides what happens
// to it next:
//   - valid message  -> published as a BusEvent on the IEventBus, so
//                       whoever is interested (a use-case, a logger, ...)
//                       can react. The router itself never decides *what*
//                       happens for a given type - that's the whole point
//                       of going through the bus instead of a direct call.
//   - malformed JSON -> an immediate ERROR reply is sent straight back via
//                       ITransport. This is a protocol-level concern (bad
//                       input), not a domain event, so it bypasses the bus.
class MessageRouter {
public:
    MessageRouter(IEventBus& bus, ITransport& transport);

    void handleRawMessage(const std::string& connectionId, const std::string& rawJson);

private:
    IEventBus& bus_;
    ITransport& transport_;
};
