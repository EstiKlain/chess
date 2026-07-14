cmake_minimum_required(VERSION 3.14)
project(chess_game CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- OpenCV ---
set(OpenCV_ROOT "${CMAKE_SOURCE_DIR}/OpenCV_451")
set(OpenCV_INCLUDE_DIRS "${OpenCV_ROOT}/include")
set(OpenCV_LIB_DIR "${OpenCV_ROOT}/bin")

add_executable(chess_game
    src/main.cpp
    src/model/Board.cpp
    src/Movement.cpp
    src/Controller.cpp
    src/BoardMapper.cpp
    src/RealTimeArbiter.cpp
    src/RuleEngine.cpp
    src/GameOverRule.cpp
    src/BoardParser.cpp
    src/BoardPrinter.cpp
    src/GameEngine.cpp
    src/PieceRules.cpp
    src/ScriptRunner.cpp
    src/img.cpp
)
target_include_directories(chess_game PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_directories(chess_game PRIVATE ${OpenCV_LIB_DIR})
target_link_libraries(chess_game PRIVATE
    debug opencv_world451d
    optimized opencv_world451
)

# Copy the correct OpenCV DLL next to the exe after build
add_custom_command(TARGET chess_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${OpenCV_LIB_DIR}/opencv_world451d.dll,${OpenCV_LIB_DIR}/opencv_world451.dll>"
        "$<TARGET_FILE_DIR:chess_game>"
)

option(BUILD_TESTS "Build the doctest test suite" ON)

if(BUILD_TESTS)
    enable_testing()

    add_executable(chess_tests
        tests/doctest_main.cpp
        tests/Config.tests.cpp
        tests/Board.tests.cpp
        tests/Movement.tests.cpp
        tests/GameEngine.tests.cpp
        tests/Controller.tests.cpp
        tests/BoardMapper.tests.cpp
        tests/RealTimeArbiter.tests.cpp
        tests/RuleEngine.tests.cpp
        tests/GameOverRule.tests.cpp
        tests/PieceRules.tests.cpp
        src/model/Board.cpp
        src/Movement.cpp
        src/Controller.cpp
        src/BoardMapper.cpp
        src/RealTimeArbiter.cpp
        src/RuleEngine.cpp
        src/GameOverRule.cpp
        src/BoardParser.cpp
        src/BoardPrinter.cpp
        src/GameEngine.cpp
        src/PieceRules.cpp
        src/ScriptRunner.cpp
        src/img.cpp
    )
    target_include_directories(chess_tests PRIVATE src third_party ${OpenCV_INCLUDE_DIRS})
    target_link_directories(chess_tests PRIVATE ${OpenCV_LIB_DIR})
    target_link_libraries(chess_tests PRIVATE
        debug opencv_world451d
        optimized opencv_world451
    )

    add_custom_command(TARGET chess_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${OpenCV_LIB_DIR}/opencv_world451d.dll,${OpenCV_LIB_DIR}/opencv_world451.dll>"
            "$<TARGET_FILE_DIR:chess_tests>"
    )

    add_test(NAME chess_tests COMMAND chess_tests)
endif()

add_executable(img_test
    manual_tests/test_img.cpp
    src/img.cpp
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