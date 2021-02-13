
set(SOURCES log.cpp)
add_library(common STATIC ${SOURCES})
target_link_libraries(common fmt)