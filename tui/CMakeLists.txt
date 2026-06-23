if (NOT DEFINED CMAKE_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyTUI)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# resolve symlinks (e.g. uv/pyenv shims) so the venv's pyvenv.cfg points at the
# real interpreter dir, otherwise it can't find its stdlib
file(REAL_PATH ${Python3_EXECUTABLE} TUI_PYTHON)

set(TUI_VENV ${CMAKE_CURRENT_SOURCE_DIR}/.venv)
set(TUI_STAMP ${TUI_VENV}/.install-stamp)

# build venv + install deps, tracked by a stamp file
add_custom_command(
    OUTPUT ${TUI_STAMP}
    COMMAND ${TUI_PYTHON} -m venv ${TUI_VENV}
    COMMAND ${TUI_VENV}/bin/pip install --quiet --upgrade pip
    COMMAND ${TUI_VENV}/bin/pip install --quiet -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    COMMAND ${CMAKE_COMMAND} -E touch ${TUI_STAMP}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    COMMENT "Setting up Zappy TUI venv"
)   

# copy wrapper to repo root as zappy_tui, after venv ready
add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zappy_tui
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/zappy_tui_wrapper
${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zappy_tui
    COMMAND chmod +x ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zappy_tui
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/zappy_tui_wrapper ${TUI_STAMP}
)   

add_custom_target(zappy_tui ALL DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zappy_tui)
