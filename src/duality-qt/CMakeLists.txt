cmake_minimum_required(VERSION 3.2)
project(Duality-Qt CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../modules)

set(SOURCES
  src/main.cpp
  src/main_window.cpp)

set(HEADERS
  src/main_window.hpp)

set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(Duality-Qt ${SOURCES} ${HEADERS})
target_link_libraries(Duality-Qt Qt5::Widgets)