# allows `cmake -S gui -B gui/build` to work standalone
if(NOT DEFINED CMAKE_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyGUI)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GUI_SOURCES
    src/main.cpp
    src/renderer/HeadlessRenderer.cpp
    src/network/TcpSocket.cpp
    src/network/ProtocolParser.cpp
    src/core/EventQueue.cpp
    src/core/GameState.cpp
    src/core/Args.cpp
    src/core/App.cpp
)

add_executable(zappy_gui ${GUI_SOURCES})

target_include_directories(zappy_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(zappy_gui PRIVATE
    -Wall -Wextra -Wpedantic
)

# Raylib — uncomment once you're ready to add it
# find_package(raylib REQUIRED)
# target_link_libraries(zappy_gui PRIVATE raylib)
