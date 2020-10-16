cmake_minimum_required(VERSION 3.2)

project(fauxDS CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  source/main.cpp)

set(HEADERS
  source/common/integer.hpp)

include_directories(source)

add_subdirectory(source/common)
add_subdirectory(source/core)
add_subdirectory(source/third_party)

add_executable(fauxDS ${SOURCES} ${HEADERS})
target_link_libraries(fauxDS core SDL2 SDL2main)