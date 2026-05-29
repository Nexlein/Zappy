#  NOT FINAL, JUST A STARTING POINT

if(NOT DEFINED CMAKE_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyTests)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# pull in GUI sources needed for protocol/state tests
set(GUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/network/TcpSocket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/network/ProtocolParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/EventQueue.cpp
)

set(TEST_SOURCES
    test_main.cpp
    test_tcpsocket.cpp
    test_eventqueue.cpp
    test_protocolparser.cpp
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
