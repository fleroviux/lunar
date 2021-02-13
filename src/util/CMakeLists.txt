cmake_minimum_required(VERSION 3.2)
project(Duality-Util CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../external)

set(SOURCES
  src/log.cpp)

set(HEADERS
)

set(HEADERS_PUBLIC
  include/util/bit.hpp
  include/util/fifo.hpp
  include/util/integer.hpp
  include/util/likely.hpp
  include/util/log.hpp
  include/util/math.hpp
  include/util/meta.hpp)

add_library(duality-util ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})
target_include_directories(duality-util PUBLIC include)
target_link_libraries(duality-util fmt)
 