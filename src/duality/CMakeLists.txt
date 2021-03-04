cmake_minimum_required(VERSION 3.2)
project(Duality-Common CXX)

set(SOURCES
)

set(HEADERS
)

set(HEADERS_PUBLIC
)

add_library(duality-common STATIC ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})
target_include_directories(duality-core PRIVATE src)
target_include_directories(duality-common PUBLIC include)