cmake_minimum_required(VERSION 3.2)
project(Duality-SDL CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../modules)

set(SOURCES
  src/device/audio_device.cpp
  src/device/video_device.cpp
  src/main.cpp)

set(HEADERS
  src/device/audio_device.hpp
  src/device/video_device.hpp)

include(FindSDL2)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(Duality-SDL ${SOURCES} ${HEADERS})
set_target_properties(Duality-SDL PROPERTIES OUTPUT_NAME "Duality")
target_include_directories(Duality-SDL PRIVATE ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
target_link_libraries(Duality-SDL duality-util duality-core ${SDL2_LIBRARY} OpenGL::GL GLEW::GLEW)