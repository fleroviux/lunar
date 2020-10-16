set(SOURCES
  arm/interpreter/cp15.cpp
  arm/interpreter/interpreter.cpp
  arm/interpreter/tablegen/tablegen.cpp)

set(HEADERS
  arm/interpreter/handlers/arithmetic.inl
  arm/interpreter/handlers/handler16.inl
  arm/interpreter/handlers/handler32.inl
  arm/interpreter/handlers/memory.inl
  arm/interpreter/tablegen/gen_arm.hpp
  arm/interpreter/tablegen/gen_thumb.hpp
  arm/interpreter/cp15.hpp
  arm/interpreter/interpreter.hpp
  arm/interpreter/state.hpp
  arm/cpu_core.hpp
  arm/decoder.hpp
  arm/memory.hpp)

add_library(core STATIC ${SOURCES} ${HEADERS})
target_link_libraries(core common fmt)