# allows `cmake -S gui -B gui/build` to work standalone
if(NOT DEFINED CMAKE_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyGUI)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch raylib
include(FetchContent)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_TRACELOG_DEBUG OFF CACHE BOOL "" FORCE)
set(SUPPORT_TRACELOG_INFO OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
)
FetchContent_MakeAvailable(raylib)

set(GUI_SOURCES
    src/main.cpp
    src/renderer/HeadlessRenderer.cpp
    src/renderer/RaylibRenderer.cpp
    src/renderer/raylib_helpers/SelectionFinder.cpp
    src/renderer/raylib_helpers/GridRenderer.cpp
    src/renderer/raylib_helpers/EntityRenderer.cpp
    src/renderer/raylib_helpers/RenderingHelper.cpp
    src/renderer/raylib_helpers/TextRenderer.cpp
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

target_link_libraries(zappy_gui PRIVATE raylib)
