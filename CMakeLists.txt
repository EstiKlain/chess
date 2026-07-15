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
    src/legacy/main.cpp
    src/model/Board.cpp
    src/rules/Movement.cpp
    src/input/Controller.cpp
    src/input/BoardMapper.cpp
    src/realtime/RealTimeArbiter.cpp
    src/rules/RuleEngine.cpp
    src/rules/GameOverRule.cpp
    src/legacy/BoardParser.cpp
    src/legacy/BoardPrinter.cpp
    src/engine/GameEngine.cpp
    src/rules/PieceRules.cpp
    src/legacy/ScriptRunner.cpp
    src/view/canvas/img.cpp
)
target_include_directories(chess_game PRIVATE src ${OpenCV_INCLUDE_DIRS})
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

# --- chess_gui: NEW - UI-Iteration A window, no engine wired in yet -------
# Deliberately does NOT link model/rules/engine sources: Iteration A draws
# an empty checkerboard only, it doesn't touch GameEngine/Controller at all.
# Those get added to this same target's source list starting Iteration D.
add_executable(chess_gui
    src/app/main_gui.cpp
    src/view/canvas/img.cpp
    src/view/canvas/ImgCanvas.cpp
    src/view/render/BoardGeometry.cpp
    src/view/render/BoardRenderer.cpp
    src/view/render/PiecePlacement.cpp
    src/view/assets/SpriteLoader.cpp
)
target_include_directories(chess_gui PRIVATE src ${OpenCV_INCLUDE_DIRS})
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
        tests/unit/Controller.tests.cpp
        tests/unit/BoardMapper.tests.cpp
        tests/unit/RealTimeArbiter.tests.cpp
        tests/unit/RuleEngine.tests.cpp
        tests/unit/GameOverRule.tests.cpp
        tests/unit/PieceRules.tests.cpp
        tests/unit/BoardGeometry.tests.cpp
        tests/unit/SpriteLoader.tests.cpp
        tests/unit/PiecePlacement.tests.cpp
        tests/unit/BoardRenderer.tests.cpp
        src/model/Board.cpp
        src/rules/Movement.cpp
        src/input/Controller.cpp
        src/input/BoardMapper.cpp
        src/realtime/RealTimeArbiter.cpp
        src/rules/RuleEngine.cpp
        src/rules/GameOverRule.cpp
        src/legacy/BoardParser.cpp
        src/legacy/BoardPrinter.cpp
        src/engine/GameEngine.cpp
        src/rules/PieceRules.cpp
        src/legacy/ScriptRunner.cpp
        src/view/canvas/img.cpp
        src/view/render/BoardGeometry.cpp
        src/view/render/BoardRenderer.cpp
        src/view/render/PiecePlacement.cpp
        src/view/assets/SpriteLoader.cpp
    )
    target_include_directories(chess_tests PRIVATE src third_party ${OpenCV_INCLUDE_DIRS})
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
target_include_directories(img_test PRIVATE src ${OpenCV_INCLUDE_DIRS})
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