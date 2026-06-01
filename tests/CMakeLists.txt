#  NOT FINAL, JUST A STARTING POINT

if(NOT DEFINED CMAKE_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyTests)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
fetchcontent_declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
fetchcontent_makeavailable(googletest)

# Silence compiler warnings from external GoogleTest targets
if(TARGET gtest)
    target_compile_options(gtest PRIVATE -w)
endif()

include(GoogleTest)

# pull in GUI sources needed for protocol/state tests
set(GUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/network/TcpSocket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/network/ProtocolParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/EventQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/GameState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/Args.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/App.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/renderer/HeadlessRenderer.cpp
)

set(TEST_SOURCES
    gui/test_main.cpp
    gui/test_tcpsocket.cpp
    gui/test_eventqueue.cpp
    gui/test_protocolparser.cpp
    gui/test_gamestate.cpp
    gui/test_args.cpp
    gui/test_app.cpp
)

add_executable(zappy_tests ${TEST_SOURCES} ${GUI_SOURCES})

target_include_directories(zappy_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src
)

target_link_libraries(zappy_tests PRIVATE
    gtest_main
)

target_compile_options(zappy_tests PRIVATE
    -Wall -Wextra
)

gtest_discover_tests(zappy_tests)

# Python AI tests
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME zappy_ai_tests
        COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/ai -p "test_*.py"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../ai
    )
    set_tests_properties(zappy_ai_tests PROPERTIES
        ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../ai/src"
    )
endif()

