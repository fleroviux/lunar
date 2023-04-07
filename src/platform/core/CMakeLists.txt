cmake_minimum_required(VERSION 3.2)
project(lunar-platform-core CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeModules)

include(FindSDL2)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

set(SOURCES
  src/device/ogl_video_device.cpp
  src/device/sdl_audio_device.cpp
  src/emulator_thread.cpp
  src/frame_limiter.cpp
)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/platform/device/ogl_video_device.hpp
  include/platform/device/sdl_audio_device.hpp
  include/platform/emulator_thread.hpp
  include/platform/frame_limiter.hpp
)

add_library(platform-core STATIC ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})
target_include_directories(platform-core PRIVATE src)
target_include_directories(platform-core PUBLIC include ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
target_link_libraries(platform-core PRIVATE lunatic)
target_link_libraries(platform-core PUBLIC lunar fmt ${SDL2_LIBRARY} OpenGL::GL GLEW::GLEW)
