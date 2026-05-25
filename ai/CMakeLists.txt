#  NOT FINAL, JUST A STARTING POINT

if(NOT DEFINED CMAKE_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyAI)
endif()

# AI has no language constraint — swap CXX for C or Python if you go that route
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(AI_SOURCES
    src/main.cpp
)

add_executable(zappy_ai ${AI_SOURCES})

target_include_directories(zappy_ai PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(zappy_ai PRIVATE
    -Wall -Wextra -Wpedantic
)
