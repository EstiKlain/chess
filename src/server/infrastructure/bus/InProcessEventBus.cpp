#include "server/infrastructure/bus/InProcessEventBus.hpp"

void InProcessEventBus::subscribe(const std::string& eventType, EventHandler handler) {
    subscribers_[eventType].push_back(std::move(handler));
}

void InProcessEventBus::publish(const BusEvent& event) {
    auto it = subscribers_.find(event.type);
    if (it == subscribers_.end()) {
        return;  // Nobody is listening for this type - not an error.
    }
    for (auto& handler : it->second) {
        handler(event);
    }
}
