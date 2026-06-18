if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyServer C CXX)
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
    src/core/Server.cpp
    src/core/Args.cpp
    src/core/World.cpp
    src/core/Scheduler.cpp
    src/protocol/AiParser.cpp
    src/protocol/GuiParser.cpp
    src/protocol/Serializer.cpp
    src/network/Listener.cpp
    src/network/Connection.cpp
    src/network/ClientManager.cpp
    src/game/GuiNotifier.cpp
    src/game/HandshakeHandler.cpp
    src/game/CommandDispatcher.cpp
    src/game/GuiHandlers.cpp
    src/game/AiHandlers.cpp
)

add_executable(zappy_server ${SERVER_SOURCES})

target_include_directories(zappy_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(zappy_server PRIVATE clap)

target_compile_options(zappy_server PRIVATE
    -Wall -Wextra -Wpedantic
)
