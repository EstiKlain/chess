#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "engine/GameEngine.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/AuthGuard.hpp"
#include "server/application/ConnectionManager.hpp"
#include "server/application/GameSession.hpp"
#include "server/application/LoginUseCase.hpp"
#include "server/application/MakeMoveUseCase.hpp"
#include "server/application/StateFanOut.hpp"
#include "server/config.hpp"
#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/infrastructure/bus/InProcessEventBus.hpp"
#include "server/infrastructure/persistence/InMemoryIdentityStore.hpp"
#include "server/infrastructure/transport/WebSocketTransport.hpp"
#include "server/protocol/Envelope.hpp"
#include "server/protocol/MessageRouter.hpp"

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Loads the same opening position chess_gui uses (same parser, same asset
// file) - one shared GameSession is enough until Iteration 7 (matchmaking)
// introduces more than one game at a time.
GameEngine loadInitialEngine() {
    const std::string boardPath = std::string(PROJECT_ROOT) + "/assets/opening_board.txt";
    const Sections sections = parseSections(readFile(boardPath));
    const RawBoard raw = parseRawGrid(sections.boardLines);
    validateBoard(raw);
    Board board = buildBoard(raw);
    return GameEngine(std::move(board), pieceRules::PieceRulesRegistry());
}

}  // namespace

int main() {
    InProcessEventBus bus;
    WebSocketTransport transport;
    ConnectionManager connections;
    InMemoryIdentityStore identities;

    // AuthGuard sits between the real bus and MessageRouter: it enforces
    // "LOGIN before anything else" without MessageRouter itself ever
    // learning that authentication exists. MakeMoveUseCase/LoginUseCase and
    // the PING/MOVE/JUMP/LOGIN subscriptions below are wired to the raw
    // `bus`, not `authGuard` - they only ever run once a request has already
    // passed the gate, so wiring them past it again would add nothing.
    AuthGuard authGuard(bus, identities, transport);
    MessageRouter router(authGuard, transport);

    GameSession session(loadInitialEngine());
    MakeMoveUseCase makeMoveUseCase(bus, transport, connections, identities);
    LoginUseCase loginUseCase(identities, transport);

    // Wiring #1: transport lifecycle -> connection registry. Rejection of a
    // 3rd connection happens HERE, at connect time, not on the first MOVE -
    // there is no Play/Room yet to give a 3rd visitor anywhere else to go.
    transport.setOnOpen([&](const std::string& id) {
        const ConnectionOutcome outcome = connections.onConnected(id, &session);
        if (!outcome.accepted) {
            transport.send(id, protocol::errorEnvelope("TABLE_FULL", "this table already has two players"));
            std::cout << "[server] connection rejected (table full): " << id << "\n";
            return;
        }
        std::cout << "[server] connection opened: " << id << " as '" << outcome.color
                  << "' (total: " << connections.connectionCount() << ")\n";
    });

    transport.setOnClose([&](const std::string& id) {
        connections.onDisconnected(id);
        // Two separate calls, deliberately not merged into one
        // ConnectionManager method: ConnectionManager's one job is
        // seat/color assignment, and folding identity cleanup into it would
        // couple two unrelated concerns into one class. Fanning out
        // disconnect cleanup across collaborators is exactly what a
        // composition root is for.
        identities.logout(id);
        std::cout << "[server] connection closed: " << id << " (total: "
                  << connections.connectionCount() << ")\n";
    });

    // Wiring #2: raw bytes in -> MessageRouter (parses, publishes on the bus,
    // or replies ERROR directly for malformed input).
    transport.setOnMessage([&](const std::string& id, const std::string& rawJson) {
        router.handleRawMessage(id, rawJson);
    });

    // Wiring #3: business logic lives in the use-case, not in these lambdas
    // (the Iteration 1 note this fixes: PONG's envelope used to be built
    // inline here, by hand - now it shares protocol::envelope() with every
    // other outgoing message type, MOVE/JUMP delegate to MakeMoveUseCase in
    // one line). PONG itself still doesn't warrant its own use-case class -
    // there is no domain/core logic behind it at all, just an envelope.
    bus.subscribe("PING", [&](const BusEvent& event) {
        transport.send(event.connectionId, protocol::envelope("PONG", nlohmann::json::object()));
    });

    bus.subscribe("LOGIN", [&](const BusEvent& event) {
        loginUseCase.handleLogin(event.connectionId, event.requestId, event.payload);
    });

    bus.subscribe("MOVE", [&](const BusEvent& event) {
        makeMoveUseCase.handleMove(event.connectionId, event.requestId, event.payload);
    });
    bus.subscribe("JUMP", [&](const BusEvent& event) {
        makeMoveUseCase.handleJump(event.connectionId, event.requestId, event.payload);
    });

    // Tick thread: the server-side equivalent of chess_gui's render-loop
    // frame delta (src/app/main_gui.cpp's `while (!canvas.shouldClose())`
    // loop) - measures real elapsed time and advances the game clock via
    // the same GameEngine::wait(ms) that loop already calls, THEN broadcasts
    // the resulting state to every connection. The broadcast half was added
    // after Iteration 3.5 manual testing showed STATE_UPDATE was only ever
    // sent from LOGIN/MOVE/JUMP - discrete events - so a networked client's
    // view of an in-flight motion never advanced between those events (no
    // animation, and a move's own visual effect only appeared once the NEXT
    // move triggered a fresh STATE_UPDATE). Broadcasting every tick, not
    // only on request-triggered events, is what makes the client's snapshot
    // (and its nowMs) advance continuously, the same way the local build's
    // per-frame engine.snapshot() call always did. Not correlated to any
    // request, so ("", "") - same convention already used by the
    // StateFanOut call after a successful login. Detached: the process has
    // no graceful shutdown path yet (transport.stop() is never called
    // anywhere today), so there is nothing meaningful to join on.
    std::thread([&session, &connections, &identities, &transport]() {
        auto lastTick = std::chrono::steady_clock::now();
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(server_config::kTickIntervalMs));
            const auto now = std::chrono::steady_clock::now();
            const long deltaMs =
                static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count());
            lastTick = now;
            session.wait(deltaMs);
            StateFanOut::broadcast(session, connections, identities, transport, "", "");
        }
    }).detach();

    std::cout << "[server] listening on port " << server_config::kPort << "...\n";
    transport.run(server_config::kPort);  // Blocks until transport.stop() is called.
    return 0;
}
