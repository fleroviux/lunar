cmake_minimum_required(VERSION 3.2)

project(fauxDS CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set(SOURCES
  source/main.cpp)

set(HEADERS
  source/common/integer.hpp)

include_directories(source)

add_subdirectory(source/common)
add_subdirectory(source/core)
add_subdirectory(source/third_party)

include(FindSDL2)
find_package(SDL2 REQUIRED)

add_executable(fauxDS ${SOURCES} ${HEADERS})
message(STATUS ${SDL2_INCLUDE_DIR})
target_include_directories(fauxDS PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(fauxDS core ${SDL2_LIBRARY})