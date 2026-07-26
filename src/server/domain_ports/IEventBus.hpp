#pragma once

#include <functional>
#include <string>

#include <nlohmann/json.hpp>

// A single event carried on the bus. "type" matches MessageEnvelope::type
// (e.g. "PING", "MOVE", later domain events like "MoveApplied").
// connectionId identifies which network connection this event relates to.
// requestId is carried through so a handler can correlate its eventual
// reply with the specific request that triggered it - it means nothing for
// events with no originating client request (e.g. published domain events
// like "MoveApplied"), where it is left empty.
// payload is the envelope's payload, passed through untouched.
struct BusEvent {
    std::string type;
    std::string connectionId;
    std::string requestId;
    nlohmann::json payload;
};

using EventHandler = std::function<void(const BusEvent&)>;

// Port: Application/protocol code depends on THIS interface only, never on
// InProcessEventBus directly. Keeps the dependency arrow pointing inward
// (infrastructure -> domain_ports), never the other way around.
class IEventBus {
public:
    virtual ~IEventBus() = default;

    // Registers a handler for a given event type. Multiple handlers may be
    // registered for the same type; all of them get called on publish().
    virtual void subscribe(const std::string& eventType, EventHandler handler) = 0;

    // Publishes an event to every handler subscribed to event.type.
    // If nobody is subscribed, this is a silent no-op.
    virtual void publish(const BusEvent& event) = 0;
};
