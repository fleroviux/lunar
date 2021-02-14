
# TODO: remove this and let each component bring its own headers instead.
include_directories(.)

add_subdirectory(../external ${CMAKE_CURRENT_BINARY_DIR}/external)
add_subdirectory(core)
add_subdirectory(duality)
add_subdirectory(util)
