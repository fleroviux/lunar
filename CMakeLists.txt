cmake_minimum_required(VERSION 3.2)

project(lunar)

option(PLATFORM_SDL2 "Build SDL2 frontend" ON)

add_subdirectory(external ${CMAKE_BINARY_DIR}/external)
add_subdirectory(src/lunar)
add_subdirectory(src/platform/core)

if (PLATFORM_SDL2)
  add_subdirectory(src/platform/sdl ${CMAKE_CURRENT_BINARY_DIR}/bin/sdl)
endif()