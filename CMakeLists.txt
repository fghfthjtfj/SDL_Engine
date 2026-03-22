cmake_minimum_required(VERSION 3.20)
project(XOR_Engine)

add_subdirectory(src/engine)
add_subdirectory(src/game)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Game)
