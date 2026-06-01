cmake_minimum_required(VERSION 3.20)
project(Zappy)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# each binary can also be built standalone from its own directory
add_subdirectory(server)
add_subdirectory(gui)
# add_subdirectory(ai)  # uncomment when AI team decides on language

# optional: build tests alongside everything
option(ZAPPY_BUILD_TESTS "Build tests" OFF)
if(ZAPPY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

