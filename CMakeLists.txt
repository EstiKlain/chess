cmake_minimum_required(VERSION 3.14)
project(chess_game CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- OpenCV ---
set(OpenCV_ROOT "${CMAKE_SOURCE_DIR}/OpenCV_451")
set(OpenCV_INCLUDE_DIRS "${OpenCV_ROOT}/include")
set(OpenCV_LIB_DIR "${OpenCV_ROOT}/bin")

# --- Server-layer third-party deps (Server-Iteration 1) -------------------
# All three are fetched at configure/build time - nothing to install by
# hand, works the same on MSVC and on gcc/clang.
include(FetchContent)

# Asio (standalone, header-only - NOT Boost.Asio). websocketpp sits on top
# of it for the actual socket/event-loop work.
FetchContent_Declare(asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-16-1)
FetchContent_MakeAvailable(asio)
# Asio ships no CMake target of its own (only examples) - so we wrap its
# headers into a tiny INTERFACE target ourselves.
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE "${asio_SOURCE_DIR}/asio/include")
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
# MSVC does not report the true __cplusplus value unless told to - without
# this, websocketpp's internal "are we on C++11?" check fails, and it falls
# back to its OLD Boost-based code path (hence "boost/aligned_storage.hpp
# not found", even though we never asked for Boost anywhere). /Zc:__cplusplus
# fixes the misreport; harmless no-op on gcc/clang.
target_compile_options(asio INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>)

# websocketpp - header-only WebSocket client/server library.
# NOTE: we deliberately use FetchContent_Populate (not MakeAvailable) here.
# MakeAvailable would run websocketpp's own CMakeLists.txt via add_subdirectory,
# and that file unconditionally does find_package(Boost) for building ITS OWN
# examples/tests - something we never need, since websocketpp is header-only
# and we wrap it ourselves as an INTERFACE target below. Populate only
# downloads the source, without ever touching its CMakeLists.txt.
FetchContent_Declare(websocketpp
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
    GIT_TAG 0.8.2)
FetchContent_Populate(websocketpp)
add_library(websocketpp INTERFACE)
target_include_directories(websocketpp INTERFACE "${websocketpp_SOURCE_DIR}")
target_link_libraries(websocketpp INTERFACE asio)

# nlohmann::json - single-header JSON library, used for MessageEnvelope.
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3)
FetchContent_MakeAvailable(json)

# Threads - websocketpp/asio need it on Linux/gcc (harmless no-op on MSVC).
find_package(Threads REQUIRED)

# libsodium (Server-Iteration 5) - chosen for ITokenGenerator's real
# implementation (SodiumTokenGenerator, randombytes_buf-backed) and,
# deliberately, to be reused unchanged for Iteration 6's password hashing
# (crypto_pwhash_str/crypto_pwhash_str_verify - Argon2id). Cross-platform on
# purpose (over e.g. Windows CNG) - this project's confirmed eventual
# deployment target is a Docker container, almost certainly Linux. Only
# chess_server links this - chess_server_tests uses a FakeTokenGenerator
# instead, exactly like it never links the real WebSocketTransport.cpp.
FetchContent_Declare(libsodium
    GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
    GIT_TAG master)
set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libsodium)

# --- chess_server: the new server executable (Server-Iteration 1) ---------
# Iteration 1: proves a message travels client -> server -> client, with
# no game logic yet (that starts in Iteration 2). See kungfu_chess_server_plan.md.
add_executable(chess_server
    src/server/main_server.cpp
    src/server/protocol/MessageRouter.cpp
    src/shared/protocol/mappers/MoveRequestMapper.cpp
    src/shared/protocol/mappers/GameSnapshotMapper.cpp
    src/server/infrastructure/bus/InProcessEventBus.cpp
    src/server/infrastructure/transport/WebSocketTransport.cpp
    src/server/infrastructure/transport/LoggingTransport.cpp
    src/server/infrastructure/persistence/InMemoryIdentityStore.cpp
    src/shared/logging/FileLogger.cpp
    src/server/application/ConnectionManager.cpp
    src/server/application/GameSession.cpp
    src/server/application/MakeMoveUseCase.cpp
    src/server/application/LoginUseCase.cpp
    src/server/application/AuthGuard.cpp
    src/server/application/StateFanOut.cpp
    src/server/application/PlayerSessionRegistry.cpp
    src/server/application/DisconnectUseCase.cpp
    src/server/application/ReconnectUseCase.cpp
    src/server/infrastructure/time/SystemClock.cpp
    src/server/infrastructure/security/SodiumTokenGenerator.cpp
    src/core/model/Board.cpp
    src/core/rules/Movement.cpp
    src/core/rules/RuleEngine.cpp
    src/core/rules/GameOverRule.cpp
    src/core/rules/PieceRules.cpp
    src/core/realtime/RealTimeArbiter.cpp
    src/core/io/BoardParser.cpp
    src/core/io/BoardPrinter.cpp
    src/core/engine/GameEngine.cpp
)
target_include_directories(chess_server PRIVATE src src/core)
target_link_libraries(chess_server PRIVATE
    websocketpp
    asio
    nlohmann_json::nlohmann_json
    Threads::Threads
    sodium
)
target_compile_definitions(chess_server PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

# --- chess_game: your existing text-mode engine, UNCHANGED ----------------
add_executable(chess_game
    src/app/main_console.cpp
    src/core/model/Board.cpp
    src/core/rules/Movement.cpp
    src/core/input/Controller.cpp
    src/core/input/BoardMapper.cpp
    src/core/realtime/RealTimeArbiter.cpp
    src/core/rules/RuleEngine.cpp
    src/core/rules/GameOverRule.cpp
    src/core/io/BoardParser.cpp
    src/core/io/BoardPrinter.cpp
    src/core/engine/GameEngine.cpp
    src/core/rules/PieceRules.cpp
    src/core/texttests/ScriptRunner.cpp
    src/view/canvas/img.cpp
)
target_include_directories(chess_game PRIVATE src src/core ${OpenCV_INCLUDE_DIRS})
target_link_directories(chess_game PRIVATE ${OpenCV_LIB_DIR})
target_link_libraries(chess_game PRIVATE
    debug opencv_world451d
    optimized opencv_world451
)

add_custom_command(TARGET chess_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${OpenCV_LIB_DIR}/opencv_world451d.dll,${OpenCV_LIB_DIR}/opencv_world451.dll>"
        "$<TARGET_FILE_DIR:chess_game>"
)

# --- chess_gui: the real GUI, now a networked client (Server-Iteration ----
# 3.5) - main_gui.cpp no longer constructs a local GameEngine at all, so
# the rule-engine sources it used to link (GameEngine/RuleEngine/
# RealTimeArbiter/PieceRules/Movement/GameOverRule/BoardParser/BoardPrinter)
# are dropped here - that logic now lives only in chess_server. Board
# itself is still needed (Controller's click hit-testing against a
# network-synced mirror board), so core/model/Board.cpp + BoardMapper stay.
add_executable(chess_gui
    src/app/main_gui.cpp
    src/client_net/application/ServerConnection.cpp
    src/client_net/application/AutoReconnector.cpp
    src/client_net/application/SnapshotCache.cpp
    src/client_net/infrastructure/WebSocketClientLink.cpp
    src/client_net/infrastructure/ClientLogger.cpp
    src/shared/logging/FileLogger.cpp
    src/view/canvas/img.cpp
    src/view/canvas/ImgCanvas.cpp
    src/view/canvas/WindowMapper.cpp
    src/view/render/BoardGeometry.cpp
    src/view/render/BoardRenderer.cpp
    src/view/render/PiecePlacement.cpp
    src/view/render/SnapshotAdapter.cpp
    src/view/render/PieceAnimator.cpp
    src/view/assets/SpriteLoader.cpp
    src/view/assets/AnimationConfig.cpp
    src/view/hud/PlayerNamesHud.cpp
    src/view/hud/DisconnectCountdownHud.cpp
    src/view/hud/ReconnectingHud.cpp
    src/core/input/BoardMapper.cpp
    src/core/input/Controller.cpp
    src/core/model/Board.cpp
    src/view/input/ClickLogger.cpp
    src/shared/protocol/mappers/GameSnapshotMapper.cpp
    src/shared/protocol/mappers/MoveRequestMapper.cpp
)
target_include_directories(chess_gui PRIVATE src src/core ${OpenCV_INCLUDE_DIRS})
target_link_directories(chess_gui PRIVATE ${OpenCV_LIB_DIR})
target_link_libraries(chess_gui PRIVATE
    debug opencv_world451d
    optimized opencv_world451
    websocketpp
    asio
    nlohmann_json::nlohmann_json
    Threads::Threads
)
target_compile_definitions(chess_gui PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

add_custom_command(TARGET chess_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${OpenCV_LIB_DIR}/opencv_world451d.dll,${OpenCV_LIB_DIR}/opencv_world451.dll>"
        "$<TARGET_FILE_DIR:chess_gui>"
)

option(BUILD_TESTS "Build the doctest test suite" ON)

if(BUILD_TESTS)
    enable_testing()
add_executable(chess_tests
        tests/unit/doctest_main.cpp
        tests/unit/Config.tests.cpp
        tests/unit/Board.tests.cpp
        tests/unit/Movement.tests.cpp
        tests/unit/GameEngine.tests.cpp
        tests/unit/GameSnapshot.tests.cpp
        tests/unit/SnapshotAdapter.tests.cpp
        tests/unit/Controller.tests.cpp
        tests/unit/BoardMapper.tests.cpp
        tests/unit/WindowMapper.tests.cpp
        tests/unit/ClickLogger.tests.cpp   
        tests/unit/RealTimeArbiter.tests.cpp
        tests/unit/RuleEngine.tests.cpp
        tests/unit/GameOverRule.tests.cpp
        tests/unit/PieceRules.tests.cpp
        tests/unit/BoardGeometry.tests.cpp
        tests/unit/SpriteLoader.tests.cpp
        tests/unit/PiecePlacement.tests.cpp
        tests/unit/BoardRenderer.tests.cpp
        tests/unit/AnimationConfig.tests.cpp
        tests/unit/PieceAnimator.tests.cpp
        src/core/model/Board.cpp
        src/core/rules/Movement.cpp
        src/core/input/Controller.cpp
        src/core/input/BoardMapper.cpp
        src/core/realtime/RealTimeArbiter.cpp
        src/core/rules/RuleEngine.cpp
        src/core/rules/GameOverRule.cpp
        src/core/io/BoardParser.cpp
        src/core/io/BoardPrinter.cpp
        src/core/engine/GameEngine.cpp
        src/core/rules/PieceRules.cpp
        src/core/texttests/ScriptRunner.cpp
        src/view/canvas/img.cpp
        src/view/canvas/WindowMapper.cpp
        src/view/render/BoardGeometry.cpp
        src/view/render/BoardRenderer.cpp
        src/view/render/PiecePlacement.cpp
        src/view/render/SnapshotAdapter.cpp
        src/view/render/PieceAnimator.cpp
        src/view/assets/SpriteLoader.cpp
        src/view/assets/AnimationConfig.cpp
        src/view/input/ClickLogger.cpp
    )
    target_include_directories(chess_tests PRIVATE src src/core third_party ${OpenCV_INCLUDE_DIRS})
    target_link_directories(chess_tests PRIVATE ${OpenCV_LIB_DIR})
    target_link_libraries(chess_tests PRIVATE
        debug opencv_world451d
        optimized opencv_world451
    )
    target_compile_definitions(chess_tests PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

    add_custom_command(TARGET chess_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${OpenCV_LIB_DIR}/opencv_world451d.dll,${OpenCV_LIB_DIR}/opencv_world451.dll>"
            "$<TARGET_FILE_DIR:chess_tests>"
    )

    add_test(NAME chess_tests COMMAND chess_tests)

    add_executable(chess_server_tests
        tests/unit/server/doctest_main.cpp
        tests/unit/server/InProcessEventBus.tests.cpp
        tests/unit/server/MessageRouter.tests.cpp
        tests/unit/server/Mappers.tests.cpp
        tests/unit/server/MakeMoveUseCase.tests.cpp
        tests/unit/server/ConnectionManager.tests.cpp
        tests/unit/server/LoginUseCase.tests.cpp
        tests/unit/server/AuthGuard.tests.cpp
        tests/unit/server/FileLogger.tests.cpp
        tests/unit/server/LoggingTransport.tests.cpp
        tests/unit/server/PlayerSessionRegistry.tests.cpp
        tests/unit/server/DisconnectReconnect.tests.cpp
        tests/unit/server/ReconnectUseCase.tests.cpp
        src/server/protocol/MessageRouter.cpp
        src/shared/protocol/mappers/MoveRequestMapper.cpp
        src/shared/protocol/mappers/GameSnapshotMapper.cpp
        src/server/infrastructure/bus/InProcessEventBus.cpp
        src/server/infrastructure/persistence/InMemoryIdentityStore.cpp
        src/server/infrastructure/transport/LoggingTransport.cpp
        src/shared/logging/FileLogger.cpp
        src/server/application/ConnectionManager.cpp
        src/server/application/GameSession.cpp
        src/server/application/MakeMoveUseCase.cpp
        src/server/application/LoginUseCase.cpp
        src/server/application/AuthGuard.cpp
        src/server/application/StateFanOut.cpp
        src/server/application/PlayerSessionRegistry.cpp
        src/server/application/DisconnectUseCase.cpp
        src/server/application/ReconnectUseCase.cpp
        src/server/infrastructure/time/SystemClock.cpp
        src/core/model/Board.cpp
        src/core/rules/Movement.cpp
        src/core/rules/RuleEngine.cpp
        src/core/rules/GameOverRule.cpp
        src/core/rules/PieceRules.cpp
        src/core/realtime/RealTimeArbiter.cpp
        src/core/io/BoardParser.cpp
        src/core/io/BoardPrinter.cpp
        src/core/engine/GameEngine.cpp
    )
    # NOTE (Iteration 2 change): unlike Iteration 1, this target now links real
    # core/ sources - the plan's test doctrine requires MakeMoveUseCase/GameSession
    # tests to run against a REAL GameEngine (only the ports - IEventBus/ITransport -
    # are faked). core/ has no OpenCV dependency of its own (only view/ and app/ do),
    # so this does not pull OpenCV into chess_server_tests.
    target_include_directories(chess_server_tests PRIVATE src src/core third_party)
    target_link_libraries(chess_server_tests PRIVATE
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    add_test(NAME chess_server_tests COMMAND chess_server_tests)

    # --- chess_client_tests: client_net/ tests (Server-Iteration 3.5) -------
    # Its own target, not folded into chess_server_tests or chess_tests:
    # client_net/ has yet another dependency footprint - no OpenCV (unlike
    # chess_tests), and no websocketpp/asio (unlike chess_server_tests,
    # and unlike chess_gui itself) since ServerConnection is tested through
    # the IServerLink fake, never a real socket. WebSocketClientLink.cpp
    # (the concrete websocketpp adapter) is deliberately NOT linked here,
    # mirroring how chess_server_tests never links WebSocketTransport.cpp.
    add_executable(chess_client_tests
        tests/unit/client_net/doctest_main.cpp
        tests/unit/client_net/ServerConnection.tests.cpp
        tests/unit/client_net/ClientLogger.tests.cpp
        tests/unit/client_net/AutoReconnector.tests.cpp
        src/client_net/application/ServerConnection.cpp
        src/client_net/application/AutoReconnector.cpp
    src/client_net/application/SnapshotCache.cpp
        src/client_net/infrastructure/ClientLogger.cpp
        src/shared/protocol/mappers/GameSnapshotMapper.cpp
        src/shared/protocol/mappers/MoveRequestMapper.cpp
    )
    target_include_directories(chess_client_tests PRIVATE src src/core third_party)
    target_link_libraries(chess_client_tests PRIVATE
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    add_test(NAME chess_client_tests COMMAND chess_client_tests)
endif()

add_executable(img_test
    manual_tests/test_img.cpp
    src/view/canvas/img.cpp
)
target_include_directories(img_test PRIVATE src src/core ${OpenCV_INCLUDE_DIRS})
target_link_directories(img_test PRIVATE ${OpenCV_LIB_DIR})
target_link_libraries(img_test PRIVATE
    debug opencv_world451d
    optimized opencv_world451
)
target_compile_definitions(img_test PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

add_custom_command(TARGET img_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${OpenCV_LIB_DIR}/opencv_world451d.dll,${OpenCV_LIB_DIR}/opencv_world451.dll>"
        "$<TARGET_FILE_DIR:img_test>"
)