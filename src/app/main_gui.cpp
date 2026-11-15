#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "app/MatchmakingConsole.hpp"
#include "client_net/application/AutoReconnector.hpp"
#include "client_net/application/ServerConnection.hpp"
#include "client_net/infrastructure/ClientLogger.hpp"
#include "client_net/infrastructure/WebSocketClientLink.hpp"
#include "config.hpp"
#include "engine/GameSnapshot.hpp"
#include "engine/SnapshotSync.hpp"
#include "input/Controller.hpp"
#include "model/Board.hpp"
#include "shared/logging/FileLogger.hpp"
#include "shared/protocol/config.hpp"
#include "shared/protocol/dto/StateUpdateDto.hpp"
#include "view/assets/AnimationConfig.hpp"
#include "view/assets/SpriteLoader.hpp"
#include "view/canvas/ImgCanvas.hpp"
#include "view/hud/DisconnectCountdownHud.hpp"
#include "view/hud/PlayerNamesHud.hpp"
#include "view/hud/ReconnectingHud.hpp"
#include "view/render/BoardGeometry.hpp"
#include "view/render/BoardRenderer.hpp"
#include "view/render/PieceAnimator.hpp"

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

namespace
{
    // view/ takes its own minimal PlayerName type, not server::protocol's
    // PlayerDto - main_gui.cpp (which already depends on protocol/ for the
    // snapshot mapping) does this one-line conversion so view/ stays
    // decoupled from any server/session concept.
    std::vector<PlayerName> toPlayerNames(const std::vector<PlayerDto> &players)
    {
        std::vector<PlayerName> names;
        names.reserve(players.size());
        for (const PlayerDto &player : players)
        {
            names.push_back(PlayerName{player.color, player.name, player.elo});
        }
        return names;
    }

    // Console-only text I/O for the username/host prompt, per the plan's
    // "shell/console only, not GUI" requirement - separate from the OpenCV
    // window opened further below.
    void openConsole()
    {
        AllocConsole();
        FILE *dummy = nullptr;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONIN$", "r", stdin);
    }
}

int main()
{
    openConsole();

    std::cout << "Server host [localhost]: ";
    std::string host;
    std::getline(std::cin, host);
    if (host.empty())
    {
        host = "localhost";
    }

    std::cout << "Username: ";
    std::string username;
    std::getline(std::cin, username);

    // Plain std::cin - typed characters are visible on screen. Masking
    // console input needs a character-at-a-time read (_getch() loop instead
    // of getline) - a real but separate UX task, deliberately not done here
    // (see docs/kungfu_chess_server_plan (1).md's Iteration 6 section).
    std::cout << "Password: ";
    std::string password;
    std::getline(std::cin, password);

    // logs/ doubles as the natural future Docker volume mount point - the
    // path is decided here, at the composition root, and nowhere else;
    // FileLogger itself never hardcodes a path (see CLAUDE.md's
    // Future-Docker-readiness note).
    std::filesystem::create_directories(std::string(PROJECT_ROOT) + "/logs");
    std::ofstream clientLogStream(std::string(PROJECT_ROOT) + "/logs/client.log", std::ios::app);
    FileLogger clientLogger(clientLogStream);

    // ClientLogger wraps the real link and is what ServerConnection actually
    // receives as `link` - ServerConnection never changes, since it was
    // always written against IServerLink&, never against WebSocketClientLink
    // by name (the client-side mirror of LoggingTransport on the server).
    WebSocketClientLink realLink;
    ClientLogger link(realLink, clientLogger);
    ServerConnection connection(link);
    try
    {
        connection.connect(host, server_config::kPort);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to connect to " << host << ": " << e.what() << '\n';
        return 1;
    }

    if (!connection.login(username, password))
    {
        std::cerr << "Login failed: " << connection.lastError() << '\n';
        return 1;
    }

    if (!waitForMatchInteractively(connection))
    {
        std::cout << "Exiting.\n";
        return 0;
    }

    // Blocks briefly - the server sends a STATE_UPDATE immediately after a
    // successful match (ServerTick's unconditional per-tick broadcast), so
    // this is a short, bounded wait, not an indefinite hang. rows/cols come
    // from this real snapshot, not a hardcoded constant - the board opens
    // at whatever size the server actually says.
    const GameSnapshot initial = connection.awaitInitialSnapshot();

    // Must be declared AFTER `connection` (and everything connection itself
    // depends on: link/realLink/clientLogger) - this is a safety
    // requirement, not just a convenient spot. C++ destroys locals in
    // reverse declaration order, so declaring it here guarantees
    // AutoReconnector's destructor (which stops and joins its background
    // worker thread) runs BEFORE connection/link/realLink start being torn
    // down. Declaring it earlier would let the worker thread call
    // connection.reconnect() on an object already mid-destruction.
    AutoReconnector reconnector(connection);

    const int cellSize = config::CELL_SIZE;

    Board mirrorBoard;
    syncBoardFromSnapshot(mirrorBoard, initial);

    // --- Controller: same call shape as local play, ServerConnection replaces GameEngine ---
    Controller controller(mirrorBoard, connection);

    // --- View setup: window is sized only now, from real network data ---
    const auto size = BoardGeometry::boardPixelSize(mirrorBoard.rows(), mirrorBoard.cols(), cellSize);
    ImgCanvas canvas(size.width, size.height, "Kung Fu Chess");

    SpriteLoader spriteLoader(std::string(PROJECT_ROOT) + "/assets/pieces_classic");

    AnimationConfigLoader animConfigLoader(std::string(PROJECT_ROOT) + "/assets/pieces_classic");
    const AnimationLookup animLookup =
        [&](const std::string &pieceCode, const std::string &state) -> AnimationSpec
    {
        const AnimationConfig &config = animConfigLoader.configFor(pieceCode, state);
        return AnimationSpec{config.framesPerSec, spriteLoader.frameCount(pieceCode, state), config.isLoop};
    };

    canvas.setOnMouseClick([&controller](int x, int y)
                           { controller.handleClick(x, y); });

    canvas.setOnRightMouseClick([&controller](int x, int y)
                                { controller.handleJumpClick(x, y); });
    const ColorRGB dark{181, 136, 99};

    while (!canvas.shouldClose())
    {
        // No local engine.wait(...) here at all - the server's own tick
        // thread already advances the real clock; the client only ever
        // displays what STATE_UPDATE tells it.
        const GameSnapshot snapshot = connection.latestSnapshot();
        syncBoardFromSnapshot(mirrorBoard, snapshot);
        // UX-only pre-check (see Controller::setLocalColor's doc comment) -
        // refreshed every tick from the latest STATE_UPDATE, same as the
        // mirror board itself, rather than set once after login.
        controller.setLocalColor(connection.myColor());

        const auto animated = PieceAnimator::computePlacements(snapshot, cellSize, animLookup);
        const auto players = toPlayerNames(connection.latestPlayers());

        canvas.clear(dark);
        BoardRenderer::drawBoard(canvas, snapshot.rows, snapshot.cols, cellSize);
        BoardRenderer::drawAnimatedPieces(canvas, spriteLoader, animated, cellSize);

        if (controller.hasSelection())
            BoardRenderer::highlightCell(canvas, controller.selectedRow(), controller.selectedCol(), cellSize);

        Hud::drawPlayerNames(canvas, players);

        if (const auto secondsLeft = connection.latestDisconnectCountdown())
            Hud::drawDisconnectCountdown(canvas, *secondsLeft);

        if (reconnector.isReconnecting())
            Hud::drawReconnecting(canvas, reconnector.secondsRemaining().value_or(0));
        else if (reconnector.hasFailed())
            Hud::drawConnectionLost(canvas);

        if (snapshot.gameOver)
            BoardRenderer::drawGameOverOverlay(canvas, size.width, size.height);

        canvas.present();
    }

    return 0;
}
