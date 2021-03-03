project(Duality)

# TODO: remove this and let each component bring its own headers instead.
include_directories(.)

add_subdirectory(../external ${CMAKE_CURRENT_BINARY_DIR}/external)
add_subdirectory(core)
add_subdirectory(util)

option(BUILD_DUALITY_SDL2 "Build the SDL2 frontend" ON)
option(BUILD_DUALITY_QT "Build the Qt frontend" OFF)

if (BUILD_DUALITY_SDL2)
  add_subdirectory(duality-sdl)
endif()

if (BUILD_DUALITY_QT)
  add_subdirectory(duality-qt)
endif()
