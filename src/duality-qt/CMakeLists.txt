cmake_minimum_required(VERSION 3.2)
project(Duality-Qt CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../modules)

set(SOURCES
  src/main.cpp
  src/main_window.cpp
  src/screen.cpp)

set(HEADERS
  src/main_window.hpp
  src/screen.hpp)

set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)

add_executable(Duality-Qt ${SOURCES} ${HEADERS})
set_target_properties(Duality-Qt PROPERTIES OUTPUT_NAME "Duality")
target_link_libraries(Duality-Qt Qt5::Widgets Qt5::OpenGL duality-util)