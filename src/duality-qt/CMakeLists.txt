cmake_minimum_required(VERSION 3.2)
project(Duality-Qt CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../cmake)

set(SOURCES
  src/audio_device.cpp
  src/main.cpp
  src/main_window.cpp
  src/screen.cpp)

set(HEADERS
  src/audio_device.hpp
  src/main_window.hpp
  src/screen.hpp)

set(CMAKE_AUTOMOC ON)
find_package(OpenGL REQUIRED)
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(Duality-Qt ${SOURCES} ${HEADERS})
set_target_properties(Duality-Qt PROPERTIES OUTPUT_NAME "Duality")
target_link_libraries(Duality-Qt Qt5::Widgets Qt5::OpenGL OpenGL::GL duality-common miniaudio Threads::Threads ${CMAKE_DL_LIBS})
