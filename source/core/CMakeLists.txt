set(SOURCES
  arm/cp15.cpp
  arm/arm.cpp
  arm/tablegen/tablegen.cpp
  arm7/bus.cpp
  arm7/bus_mmio.cpp
  arm9/bus.cpp
  arm9/bus_mmio.cpp
  hw/video_unit/video_unit.cpp
  core.cpp
  scheduler.cpp)

set(HEADERS
  arm/handlers/arithmetic.inl
  arm/handlers/handler16.inl
  arm/handlers/handler32.inl
  arm/handlers/memory.inl
  arm/tablegen/gen_arm.hpp
  arm/tablegen/gen_thumb.hpp
  arm/cp15.hpp
  arm/arm.hpp
  arm/state.hpp
  arm/decoder.hpp
  arm/memory.hpp
  arm7/bus.hpp
  arm9/bus.hpp
  hw/video_unit/video_unit.hpp
  core.hpp
  interconnect.hpp
  scheduler.hpp)

add_library(core STATIC ${SOURCES} ${HEADERS})
target_link_libraries(core common fmt)