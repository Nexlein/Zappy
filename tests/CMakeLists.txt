#  NOT FINAL, JUST A STARTING POINT

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyTests C CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GTest: to install locally: sudo apt install libgtest-dev
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found on system — fetching via FetchContent\n"
                    "   To avoid this next time: sudo apt install libgtest-dev")
    include(FetchContent)
    fetchcontent_declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    fetchcontent_makeavailable(googletest)
    if(TARGET gtest)
        target_compile_options(gtest PRIVATE -w)
    endif()
    set(GTEST_TARGET gtest_main)
else()
    message(STATUS "Found system GTest ${GTest_VERSION}")
    set(GTEST_TARGET GTest::gtest_main)
endif()

include(GoogleTest)

if(ZAPPY_BUILD_GUI)
    # pull in GUI sources needed for protocol/state tests
    set(GUI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/network/TcpSocket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/network/ProtocolParser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/EventQueue.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/GameState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/Args.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/behaviors/MoveBehavior.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/behaviors/TurnBehavior.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/behaviors/DeathBehavior.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/behaviors/ForkBehavior.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/behaviors/IncantationBehavior.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/behaviors/LevelUpBehavior.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/core/behaviors/BroadcastBehavior.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/renderer/raylib_helpers/RenderingHelper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/renderer/raylib_helpers/TileSlotMap.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src/renderer/raylib_helpers/I18n.cpp
    )

    set(TEST_SOURCES
        gui/test_main.cpp
        gui/test_tcpsocket.cpp
        gui/test_eventqueue.cpp
        gui/test_protocolparser.cpp
        gui/test_gamestate.cpp
        gui/test_args.cpp
        gui/test_tileslotmap.cpp
        gui/test_i18n.cpp
    )

    add_executable(zappy_gui_tests ${TEST_SOURCES} ${GUI_SOURCES})

    target_include_directories(zappy_gui_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/src
    )

    target_link_libraries(zappy_gui_tests PRIVATE
        ${GTEST_TARGET}
        raylib
    )

    target_compile_options(zappy_gui_tests PRIVATE
        -Wall -Wextra
    )

    gtest_discover_tests(zappy_gui_tests)
endif()

if(ZAPPY_BUILD_SERVER)
    set(SERVER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/core/World.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/core/Args.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/core/Scheduler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/core/GameClock.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/lib/clap/App.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/protocol/AiParser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/protocol/GuiParser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/protocol/Serializer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/network/Listener.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/network/Connection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/network/ClientManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/game/GuiNotifier.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/game/HandshakeHandler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/game/CommandDispatcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/game/GuiHandlers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src/game/AiHandlers.cpp
    )

    set(SERVER_TEST_SOURCES
        server/test_world.cpp
        server/test_args.cpp
        server/test_resources.cpp
        server/test_scheduler.cpp
        server/test_game_clock.cpp
        server/test_aiparser.cpp
        server/test_guiparser.cpp
        server/test_serializer.cpp
        server/test_listener.cpp
        server/test_connection.cpp
        server/test_client_manager.cpp
        server/test_gui_notifier.cpp
        server/test_handshake_handler.cpp
        server/test_command_dispatcher.cpp
        server/test_gui_handlers.cpp
        server/test_ai_handlers.cpp
    )

    add_executable(zappy_server_tests ${SERVER_TEST_SOURCES} ${SERVER_SOURCES})

    target_include_directories(zappy_server_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../server/lib
    )

    target_link_libraries(zappy_server_tests PRIVATE
        ${GTEST_TARGET}
    )

    target_compile_options(zappy_server_tests PRIVATE
        -Wall -Wextra
    )

    gtest_discover_tests(zappy_server_tests)
endif()

if(ZAPPY_BUILD_SERVER)
    # E2E tests: spawn the real zappy_server binary over TCP, no server internals linked.
    add_executable(zappy_server_e2e_tests
        e2e/test_e2e_handshake.cpp
        e2e/test_e2e_ai_command.cpp
        e2e/test_e2e_gui.cpp
        e2e/test_e2e_incantation.cpp
        e2e/test_e2e_starvation.cpp
        e2e/test_e2e_slot_exhaustion.cpp
    )

    target_link_libraries(zappy_server_e2e_tests PRIVATE
        ${GTEST_TARGET}
    )

    target_compile_options(zappy_server_e2e_tests PRIVATE
        -Wall -Wextra
    )

    # $<TARGET_FILE:...> gives the absolute path to the binary so CTest finds
    # it regardless of working directory. WORKING_DIRECTORY is CMAKE_SOURCE_DIR
    # so that ./zappy_server resolves correctly when the tests spawn it.
    add_test(NAME zappy_server_e2e_tests
        COMMAND $<TARGET_FILE:zappy_server_e2e_tests>
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

if(ZAPPY_BUILD_AI)
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
endif()

if(ZAPPY_BUILD_TUI)
    # Python TUI tests (supervisor layer — pure logic, no Textual UI)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME zappy_tui_tests
            COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tui -p "test_*.py"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../tui
        )
        set_tests_properties(zappy_tui_tests PROPERTIES
            ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../tui/src"
        )
    endif()
endif()
