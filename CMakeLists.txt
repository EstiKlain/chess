cmake_minimum_required(VERSION 3.14)
project(chess_game CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- OpenCV ---
set(OpenCV_ROOT "${CMAKE_SOURCE_DIR}/OpenCV_451")
set(OpenCV_INCLUDE_DIRS "${OpenCV_ROOT}/include")
set(OpenCV_LIB_DIR "${OpenCV_ROOT}/bin")

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

# --- chess_gui: the real GUI, wired to GameEngine/Controller as of ---------
# Iteration D. Iterations A-C deliberately did NOT link model/rules/engine
# sources (they drew from a flat CSV, no logic at all); this target's
# source list now includes the same core/ sources chess_game (text mode)
# links, so both executables share one GameEngine, not two.
add_executable(chess_gui
    src/app/main_gui.cpp
    src/view/canvas/img.cpp
    src/view/canvas/ImgCanvas.cpp
    src/view/render/BoardGeometry.cpp
    src/view/render/BoardRenderer.cpp
    src/view/render/PiecePlacement.cpp
    src/view/render/SnapshotAdapter.cpp
    src/view/render/PieceAnimator.cpp
    src/view/assets/SpriteLoader.cpp
    src/view/assets/AnimationConfig.cpp
    src/core/input/BoardMapper.cpp
    src/core/input/Controller.cpp
    src/core/model/Board.cpp
    src/core/rules/Movement.cpp
    src/core/rules/RuleEngine.cpp
    src/core/rules/GameOverRule.cpp
    src/core/rules/PieceRules.cpp
    src/core/realtime/RealTimeArbiter.cpp
    src/core/io/BoardParser.cpp
    src/core/io/BoardPrinter.cpp
    src/core/engine/GameEngine.cpp
    src/view/input/ClickLogger.cpp
)
target_include_directories(chess_gui PRIVATE src src/core ${OpenCV_INCLUDE_DIRS})
target_link_directories(chess_gui PRIVATE ${OpenCV_LIB_DIR})
target_link_libraries(chess_gui PRIVATE
    debug opencv_world451d
    optimized opencv_world451
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