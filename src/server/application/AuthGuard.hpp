#pragma once

#include <string>
#include <unordered_set>

#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"

// Decorator: implements IEventBus itself, wrapping the real bus. Wired in
// between InProcessEventBus and MessageRouter in main_server.cpp, so
// MessageRouter needs ZERO changes - it still just talks to an IEventBus&,
// unaware it is now talking to a guard instead of the raw bus. Enforces
// "LOGIN must happen before any other message type". A rejection here has
// exactly one interested party (the sender), so - same precedent as
// MessageRouter's own malformed-JSON handling - it bypasses the bus and
// replies via ITransport directly, rather than becoming a business event
// nobody else needs to know about.
class AuthGuard : public IEventBus {
public:
    AuthGuard(IEventBus& realBus, IIdentityStore& identities, ITransport& transport);

    /// Forwards to the wrapped bus untouched - AuthGuard only intercepts publish(), it is not itself a place anyone subscribes.
    void subscribe(const std::string& eventType, EventHandler handler) override;

    /// If event.type is exempt (LOGIN, PING) or event.connectionId is already logged in, forwards to the wrapped bus. Otherwise sends AUTH_REQUIRED directly to event.connectionId and does not forward.
    void publish(const BusEvent& event) override;

private:
    IEventBus& realBus_;
    IIdentityStore& identities_;
    ITransport& transport_;
};
