cmake_minimum_required(VERSION 3.2)
project(Duality CXX)

# TODO: move this into the components
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../modules)

# TODO: remove this and let each component bring its own headers instead.
include_directories(.)

add_subdirectory(../external ${CMAKE_CURRENT_BINARY_DIR}/external)
add_subdirectory(core)
add_subdirectory(duality)
add_subdirectory(util)
