set(SOURCES
  arm/interpreter/cp15.cpp
  arm/interpreter/arm.cpp
  arm/interpreter/tablegen/tablegen.cpp
  arm7/bus.cpp
  arm7/bus_mmio.cpp
  arm9/bus.cpp
  arm9/bus_mmio.cpp
  hw/video_unit/video_unit.cpp
  core.cpp
  scheduler.cpp)

set(HEADERS
  arm/interpreter/handlers/arithmetic.inl
  arm/interpreter/handlers/handler16.inl
  arm/interpreter/handlers/handler32.inl
  arm/interpreter/handlers/memory.inl
  arm/interpreter/tablegen/gen_arm.hpp
  arm/interpreter/tablegen/gen_thumb.hpp
  arm/interpreter/cp15.hpp
  arm/interpreter/arm.hpp
  arm/interpreter/state.hpp
  arm/cpu_core.hpp
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