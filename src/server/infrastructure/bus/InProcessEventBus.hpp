#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "server/domain_ports/IEventBus.hpp"

// Concrete IEventBus: synchronous, in-memory pub/sub. No threads, no queue -
// publish() calls every matching subscriber immediately, on the caller's
// thread. That is enough for iteration 1 (and, deliberately, for the
// foreseeable iterations after it - revisit only if something genuinely
// needs cross-thread delivery).
class InProcessEventBus : public IEventBus {
public:
    void subscribe(const std::string& eventType, EventHandler handler) override;
    void publish(const BusEvent& event) override;

private:
    std::unordered_map<std::string, std::vector<EventHandler>> subscribers_;
};
