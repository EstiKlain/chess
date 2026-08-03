#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <asio.hpp>
#include <nlohmann/json.hpp>

#include "engine/GameEngine.hpp"
#include "io/BoardParser.hpp"
#include "model/Board.hpp"
#include "rules/PieceRules.hpp"
#include "server/application/AuthGuard.hpp"
#include "server/application/ConnectionLifecycleUseCase.hpp"
#include "server/application/ConnectionManager.hpp"
#include "server/application/DisconnectUseCase.hpp"
#include "server/application/EloUpdateUseCase.hpp"
#include "server/application/GameSessionStore.hpp"
#include "server/application/LoginUseCase.hpp"
#include "server/application/MakeMoveUseCase.hpp"
#include "server/application/MatchStarter.hpp"
#include "server/application/PlayRequestUseCase.hpp"
#include "server/application/PlayerSessionRegistry.hpp"
#include "server/application/ReconnectUseCase.hpp"
#include "server/application/ServerTick.hpp"
#include "server/config.hpp"
#include "server/domain_ports/IClock.hpp"
#include "server/domain_ports/IEventBus.hpp"
#include "server/domain_ports/IIdentityStore.hpp"
#include "server/domain_ports/IPasswordHasher.hpp"
#include "server/domain_ports/IScheduler.hpp"
#include "server/domain_ports/ITokenGenerator.hpp"
#include "server/domain_ports/ITransport.hpp"
#include "server/domain_ports/IUserRepository.hpp"
#include "server/infrastructure/bus/InProcessEventBus.hpp"
#include "server/infrastructure/matchmaking/InMemoryMatchmakingQueue.hpp"
#include "server/infrastructure/persistence/InMemoryIdentityStore.hpp"
#include "server/infrastructure/persistence/InMemoryUserRepository.hpp"
#include "server/infrastructure/security/SodiumPasswordHasher.hpp"
#include "server/infrastructure/security/SodiumTokenGenerator.hpp"
#include "server/infrastructure/time/AsioScheduler.hpp"
#include "server/infrastructure/time/SystemClock.hpp"
#include "server/infrastructure/transport/LoggingTransport.hpp"
#include "server/infrastructure/transport/WebSocketTransport.hpp"
#include "server/protocol/Envelope.hpp"
#include "server/protocol/MessageRouter.hpp"
#include "shared/logging/FileLogger.hpp"
#include "shared/logging/ILogger.hpp"
#include "shared/protocol/config.hpp"

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
// file). Passed to GameSessionStore as a factory - called once per match, so
// file IO never happens on a request-handling path.
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

    // logs/ doubles as the natural future Docker volume mount point - the
    // path is decided here, at the composition root, and nowhere else;
    // FileLogger itself never hardcodes a path (see CLAUDE.md's
    // Future-Docker-readiness note).
    std::filesystem::create_directories(std::string(PROJECT_ROOT) + "/logs");
    std::ofstream serverLogStream(std::string(PROJECT_ROOT) + "/logs/server.log", std::ios::app);
    FileLogger serverLogger(serverLogStream);

    // Shared by WebSocketTransport and AsioScheduler below, so message
    // handling and the tick run on one thread - no lock needed on
    // ConnectionManager/PlayerSessionRegistry (see CLAUDE.md's Iteration 7
    // threading decision).
    asio::io_context ioContext;

    // LoggingTransport wraps the real transport and is what everyone below
    // actually receives as `transport` - AuthGuard/MessageRouter/the
    // use-cases/the tick callback never change, since they were always
    // written against ITransport&, never against WebSocketTransport by
    // name (the same trick already used for AuthGuard wrapping the bus).
    WebSocketTransport realTransport(&ioContext);
    LoggingTransport transport(realTransport, serverLogger);

    ConnectionManager connections;
    InMemoryIdentityStore identities;
    SodiumTokenGenerator tokenGenerator;
    PlayerSessionRegistry sessions(tokenGenerator);
    SystemClock clock;

    // Temporary IUserRepository stand-in - see
    // docs/kungfu_chess_server_plan (1).md's Iteration 6 section for why
    // this is InMemoryUserRepository and not PostgresUserRepository today,
    // and CMakeLists.txt for the FetchContent block deferred to the Docker
    // phase. Swap point when Docker lands: replace this one line (plus the
    // CMakeLists.txt registration) - nothing downstream names this
    // concrete type, only IUserRepository&.
    InMemoryUserRepository users;
    SodiumPasswordHasher passwordHasher;

    // AuthGuard sits between the real bus and MessageRouter: it enforces
    // "LOGIN before anything else" without MessageRouter itself ever
    // learning that authentication exists. MakeMoveUseCase/LoginUseCase and
    // the PING/MOVE/JUMP/LOGIN/RECONNECT/PLAY subscriptions below are wired
    // to the raw `bus`, not `authGuard` - they only ever run once a request
    // has already passed the gate, so wiring them past it again would add
    // nothing.
    AuthGuard authGuard(bus, identities, transport);
    MessageRouter router(authGuard, transport);

    // Owns every GameSession for the process's lifetime - PLAY creates a new
    // one per match instead of the single hardcoded session every prior
    // iteration ran against.
    GameSessionStore sessionStore(loadInitialEngine);
    InMemoryMatchmakingQueue matchQueue;

    MakeMoveUseCase makeMoveUseCase(bus, transport, connections, identities);
    LoginUseCase loginUseCase(identities, transport, users, passwordHasher);
    DisconnectUseCase disconnectUseCase(sessions, connections, transport, clock);
    ReconnectUseCase reconnectUseCase(sessions, connections, identities, transport, users);
    EloUpdateUseCase eloUpdateUseCase(users, sessions, connections);

    // The only place GameSessionStore is actually used - PlayRequestUseCase
    // itself never includes GameSessionStore.hpp, only this callback does
    // (see PlayRequestUseCase::MatchStartHandler's own doc comment).
    PlayRequestUseCase playRequestUseCase(
        matchQueue, connections, sessions, identities, users, transport, clock,
        [&](const WaitingPlayer& waiting, const WaitingPlayer& joining) {
            MatchStarter::start(sessionStore, connections, sessions, transport, waiting, joining);
        });

    ConnectionLifecycleUseCase connectionLifecycle(connections, identities, disconnectUseCase, playRequestUseCase);
    ServerTick serverTick(sessionStore, disconnectUseCase, eloUpdateUseCase, connections, identities, transport,
                           clock);

    // Wiring #1: transport lifecycle. A freshly opened connection starts
    // unbound - no session, not queued - until it LOGINs and sends PLAY (or
    // RECONNECTs with a valid token). There is no "table" to reject a 3rd
    // visitor from anymore; TABLE_FULL is retired.
    transport.setOnOpen([&](const std::string& id) {
        std::cout << "[server] connection opened: " << id << "\n";
    });

    transport.setOnClose([&](const std::string& id) {
        connectionLifecycle.onDisconnected(id);
        std::cout << "[server] connection closed: " << id << "\n";
    });

    // Wiring #2: raw bytes in -> MessageRouter (parses, publishes on the bus,
    // or replies ERROR directly for malformed input).
    transport.setOnMessage([&](const std::string& id, const std::string& rawJson) {
        router.handleRawMessage(id, rawJson);
    });

    // Wiring #3: business logic lives in the use-case, not in these lambdas.
    bus.subscribe("PING", [&](const BusEvent& event) {
        transport.send(event.connectionId, protocol::envelope("PONG", nlohmann::json::object()));
    });

    bus.subscribe("LOGIN", [&](const BusEvent& event) {
        loginUseCase.handleLogin(event.connectionId, event.requestId, event.payload);
    });

    bus.subscribe("PLAY", [&](const BusEvent& event) {
        playRequestUseCase.handlePlay(event.connectionId, event.requestId, event.payload);
    });

    bus.subscribe("MOVE", [&](const BusEvent& event) {
        makeMoveUseCase.handleMove(event.connectionId, event.requestId, event.payload);
    });
    bus.subscribe("JUMP", [&](const BusEvent& event) {
        makeMoveUseCase.handleJump(event.connectionId, event.requestId, event.payload);
    });
    bus.subscribe("RECONNECT", [&](const BusEvent& event) {
        reconnectUseCase.handleReconnect(event.connectionId, event.requestId, event.payload);
    });

    // Two independent periodic jobs, not one: ServerTick's game-session loop
    // has real ordering invariants (see its own header comment).
    // PlayRequestUseCase's matchmaking timeout has no dependency on any of
    // that - a queued player isn't in a GameSession yet - so it gets its own
    // registration instead of being folded into ServerTick.
    AsioScheduler scheduler(ioContext);
    scheduler.schedulePeriodic(server_config::kTickIntervalMs,
                                [&](long deltaMs) { serverTick.tick(deltaMs); });
    scheduler.schedulePeriodic(server_config::kTickIntervalMs,
                                [&](long) { playRequestUseCase.tick(clock.nowMs()); });

    std::cout << "[server] listening on port " << server_config::kPort << "...\n";
    transport.run(server_config::kPort);  // Blocks until transport.stop() is called.
    return 0;
}
