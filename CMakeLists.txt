cmake_minimum_required(VERSION 3.20)
project(CMake_test)

add_subdirectory(src/engine)
add_subdirectory(src/game)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Game)
