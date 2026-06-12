#  NOT FINAL, JUST A STARTING POINT

if(NOT DEFINED CMAKE_PROJECT_NAME)
    cmake_minimum_required(VERSION 3.20)
    project(ZappyAI)
endif()

# AI -> Python
add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zappy_ai
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/zappy_ai_wrapper ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zappy_ai
    COMMAND chmod +x ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zappy_ai
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/zappy_ai_wrapper
)

add_custom_target(zappy_ai ALL DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zappy_ai)
