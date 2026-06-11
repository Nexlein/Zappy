cmake_minimum_required(VERSION 3.20...4.3)
project(Zappy C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

option(ZAPPY_BUILD_SERVER "Build Server component" ON)
option(ZAPPY_BUILD_GUI "Build GUI component" ON)
option(ZAPPY_BUILD_AI "Build AI component" ON)

# each binary can also be built standalone from its own directory
if(ZAPPY_BUILD_SERVER)
    add_subdirectory(server)
endif()

if(ZAPPY_BUILD_GUI)
    add_subdirectory(gui)
endif()

if(ZAPPY_BUILD_AI)
    add_subdirectory(ai)
endif()

# optional: build tests alongside everything
option(ZAPPY_BUILD_TESTS "Build tests" OFF)
if(ZAPPY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

