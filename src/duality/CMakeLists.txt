cmake_minimum_required(VERSION 3.2)
project(Duality CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../modules)

set(SOURCES
  src/main.cpp)

set(HEADERS
)

include(FindSDL2)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(Duality ${SOURCES} ${HEADERS})
target_include_directories(Duality PRIVATE ${SDL2_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
target_link_libraries(Duality duality-util duality-core ${SDL2_LIBRARY} OpenGL::GL GLEW::GLEW)