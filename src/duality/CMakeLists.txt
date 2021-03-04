cmake_minimum_required(VERSION 3.2)
project(Duality-Common CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../modules)

set(SOURCES
  src/emulator_thread.cpp
  src/frame_limiter.cpp)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/duality/emulator_thread.hpp
  include/duality/frame_limiter.hpp)

add_library(duality-common STATIC ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})
target_link_libraries(duality-common duality-core)
target_include_directories(duality-core PRIVATE src)
target_include_directories(duality-common PUBLIC include)