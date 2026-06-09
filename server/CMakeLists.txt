if(NOT DEFINED CMAKE_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyServer)
endif()

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(clap STATIC
    lib/clap/App.cpp
)
target_include_directories(clap PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
target_compile_options(clap PRIVATE -Wall -Wextra)

set(SERVER_SOURCES
    src/main.cpp
    src/core/Args.cpp
)

add_executable(zappy_server ${SERVER_SOURCES})

target_include_directories(zappy_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(zappy_server PRIVATE clap)

target_compile_options(zappy_server PRIVATE
    -Wall -Wextra -Wpedantic
)
