cmake_minimum_required(VERSION 3.2)

project(Duality CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set(SOURCES
  source/main.cpp)

set(HEADERS
  source/common/integer.hpp)

# TODO: eventually get rid of this....
include_directories(source)

include(FindSDL2)
find_package(SDL2 REQUIRED)

add_subdirectory(source/common)
add_subdirectory(source/core)
add_subdirectory(source/third_party)

add_executable(Duality ${SOURCES} ${HEADERS})
target_include_directories(Duality PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(Duality common core ${SDL2_LIBRARY})