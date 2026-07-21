#include <cstdint>
#include <iostream>

#include <nlohmann/json.hpp>

#include "server/application/ConnectionManager.hpp"
#include "server/config.hpp"
#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/infrastructure/bus/InProcessEventBus.hpp"
#include "server/infrastructure/transport/WebSocketTransport.hpp"
#include "server/protocol/MessageRouter.hpp"

int main() {
    InProcessEventBus bus;
    WebSocketTransport transport;
    ConnectionManager connections;
    MessageRouter router(bus, transport);

    // Wiring #1: transport lifecycle -> connection registry.
    transport.setOnOpen([&](const std::string& id) {
        connections.onConnected(id);
        std::cout << "[server] connection opened: " << id << " (total: "
                  << connections.connectionCount() << ")\n";
    });

    transport.setOnClose([&](const std::string& id) {
        connections.onDisconnected(id);
        std::cout << "[server] connection closed: " << id << " (total: "
                  << connections.connectionCount() << ")\n";
    });

    // Wiring #2: raw bytes in -> MessageRouter (parses, publishes on the bus,
    // or replies ERROR directly for malformed input).
    transport.setOnMessage([&](const std::string& id, const std::string& rawJson) {
        router.handleRawMessage(id, rawJson);
    });

    // Wiring #3: the only "business logic" 
    bus.subscribe("PING", [&](const BusEvent& event) {
        const nlohmann::json pong = {
            {"type", "PONG"},
            {"requestId", ""},
            {"payload", nlohmann::json::object()},
        };
        transport.send(event.connectionId, pong.dump());
    });

    std::cout << "[server] listening on port " << server_config::kPort << "...\n";
    transport.run(server_config::kPort);  // Blocks until transport.stop() is called.
    return 0;
}
