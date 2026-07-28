#include "server/application/AuthGuard.hpp"

#include <utility>

#include "server/protocol/Envelope.hpp"

namespace {

// PING is exempt alongside LOGIN so a client can health-check the
// connection before/without logging in - it is a harmless no-op keepalive
// with no domain effect either way. RECONNECT is exempt too (Server-
// Iteration 5): a reconnecting client's new connectionId was never logged
// in, so without this exemption RECONNECT itself would be rejected before
// ReconnectUseCase ever runs its own identities.login(...) call.
const std::unordered_set<std::string> kAuthExempt = {"LOGIN", "PING", "RECONNECT"};

}  // namespace

AuthGuard::AuthGuard(IEventBus& realBus, IIdentityStore& identities, ITransport& transport)
    : realBus_(realBus), identities_(identities), transport_(transport) {}

void AuthGuard::subscribe(const std::string& eventType, EventHandler handler) {
    realBus_.subscribe(eventType, std::move(handler));
}

void AuthGuard::publish(const BusEvent& event) {
    if (kAuthExempt.count(event.type) > 0 || identities_.isLoggedIn(event.connectionId)) {
        realBus_.publish(event);
        return;
    }
    protocol::sendError(transport_, event.connectionId, event.requestId, "AUTH_REQUIRED",
                         "log in before sending " + event.type);
}
