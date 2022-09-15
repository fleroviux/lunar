cmake_minimum_required(VERSION 3.2)
project(lunar-sdl2 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/main.cpp
)

set(HEADERS
)

add_executable(lunar-sdl2 ${SOURCES} ${HEADERS})
target_link_libraries(lunar-sdl2 PRIVATE lunar platform-core)
set_target_properties(lunar-sdl2 PROPERTIES OUTPUT_NAME "lunar")