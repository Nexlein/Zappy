#  NOT FINAL, JUST A STARTING POINT

if(NOT DEFINED CMAKE_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyServer)
endif()

enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SERVER_SOURCES
    src/main.c
)

add_executable(zappy_server ${SERVER_SOURCES})

target_include_directories(zappy_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(zappy_server PRIVATE
    -Wall -Wextra -Wpedantic
)
