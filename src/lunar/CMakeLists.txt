cmake_minimum_required(VERSION 3.2)
project(lunar-core CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
  src/arm/tablegen/tablegen.cpp
  src/arm/arm.cpp
  src/common/scheduler.cpp
  src/nds/arm7/apu/apu.cpp
  src/nds/arm7/bus/bus.cpp
  src/nds/arm7/bus/io.cpp
  src/nds/arm7/dma/dma.cpp
  src/nds/arm7/rtc/rtc.cpp
  src/nds/arm7/spi/tsc/tsc.cpp
  src/nds/arm7/spi/spi.cpp
  src/nds/arm7/wifi/wifi.cpp
  src/nds/arm7/arm7.cpp
  src/nds/arm9/dma/dma.cpp
  src/nds/arm9/bus/bus.cpp
  src/nds/arm9/bus/io.cpp
  src/nds/arm9/math/math.cpp
  src/nds/arm9/arm9.cpp
  src/nds/arm9/cp15.cpp
  src/nds/cart/backup/eeprom.cpp
  src/nds/cart/backup/eeprom512b.cpp
  src/nds/cart/backup/flash.cpp
  src/nds/cart/cart.cpp
  src/nds/ipc/ipc.cpp
  src/nds/irq/irq.cpp
  src/nds/keypad/keypad.cpp
  src/nds/timer/timer.cpp
  src/nds/video_unit/gpu/renderer/opengl/opengl_renderer.cpp
  src/nds/video_unit/gpu/renderer/opengl/texture_cache.cpp
  src/nds/video_unit/gpu/renderer/software/edge_marking.cpp
  src/nds/video_unit/gpu/renderer/software/rasterizer.cpp
  src/nds/video_unit/gpu/renderer/software/software_renderer.cpp
  src/nds/video_unit/gpu/renderer/software/texture.cpp
  src/nds/video_unit/gpu/commands.cpp
  src/nds/video_unit/gpu/gpu.cpp
  src/nds/video_unit/gpu/io.cpp
  src/nds/video_unit/gpu/primitive_assembly.cpp
  src/nds/video_unit/ppu/render/affine.cpp
  src/nds/video_unit/ppu/render/oam.cpp
  src/nds/video_unit/ppu/render/text.cpp
  src/nds/video_unit/ppu/render/window.cpp
  src/nds/video_unit/ppu/composer.cpp
  src/nds/video_unit/ppu/ppu.cpp
  src/nds/video_unit/ppu/registers.cpp
  src/nds/video_unit/video_unit.cpp
  src/nds/video_unit/vram.cpp
  src/nds/core.cpp
  src/nds/exmemcnt.cpp
  src/nds/swram.cpp
)

set(HEADERS
  src/arm/handlers/arithmetic.inl
  src/arm/handlers/handler16.inl
  src/arm/handlers/handler32.inl
  src/arm/handlers/memory.inl
  src/arm/tablegen/decoder.hpp
  src/arm/tablegen/gen_arm.hpp
  src/arm/tablegen/gen_thumb.hpp
  src/arm/arm.hpp
  src/arm/state.hpp
  src/common/ogl/buffer_object.hpp
  src/common/ogl/frame_buffer_object.hpp
  src/common/ogl/program_object.hpp
  src/common/ogl/shader_object.hpp
  src/common/ogl/texture_2d.hpp
  src/common/ogl/vertex_array_object.hpp
  src/common/backup_file.hpp
  src/common/fifo.hpp
  src/common/likely.hpp
  src/common/scheduler.hpp
  src/common/static_vec.hpp
  src/nds/arm7/apu/apu.hpp
  src/nds/arm7/bus/bus.hpp
  src/nds/arm7/dma/dma.hpp
  src/nds/arm7/rtc/rtc.hpp
  src/nds/arm7/spi/tsc/tsc.hpp
  src/nds/arm7/spi/spi.hpp
  src/nds/arm7/spi/spi_device.hpp
  src/nds/arm7/wifi/wifi.hpp
  src/nds/arm7/arm7.hpp
  src/nds/arm9/bus/bus.hpp
  src/nds/arm9/dma/dma.hpp
  src/nds/arm9/math/math.hpp
  src/nds/arm9/arm9.hpp
  src/nds/arm9/cp15.hpp
  src/nds/cart/backup/eeprom.hpp
  src/nds/cart/backup/eeprom512b.hpp
  src/nds/cart/backup/flash.hpp
  src/nds/cart/cart.hpp
  src/nds/ipc/ipc.hpp
  src/nds/irq/irq.hpp
  src/nds/keypad/keypad.hpp
  src/nds/timer/timer.hpp
  src/nds/video_unit/gpu/renderer/opengl/opengl_renderer.hpp
  src/nds/video_unit/gpu/renderer/opengl/texture_cache.hpp
  src/nds/video_unit/gpu/renderer/software/edge.hpp
  src/nds/video_unit/gpu/renderer/software/software_renderer.hpp
  src/nds/video_unit/gpu/renderer/renderer_base.hpp
  src/nds/video_unit/gpu/color.hpp
  src/nds/video_unit/gpu/fixed_point.hpp
  src/nds/video_unit/gpu/gpu.hpp
  src/nds/video_unit/gpu/matrix.hpp
  src/nds/video_unit/gpu/matrix_stack.hpp
  src/nds/video_unit/gpu/vector.hpp
  src/nds/video_unit/ppu/ppu.hpp
  src/nds/video_unit/ppu/registers.hpp
  src/nds/video_unit/video_unit.hpp
  src/nds/video_unit/vram.hpp
  src/nds/video_unit/vram_region.hpp
  src/nds/exmemcnt.hpp
  src/nds/interconnect.hpp
  src/nds/swram.hpp
)

set(HEADERS_PUBLIC
  include/lunar/device/audio_device.hpp
  include/lunar/device/input_device.hpp
  include/lunar/device/video_device.hpp
  include/lunar/core.hpp
)

add_library(lunar STATIC ${SOURCES} ${HEADERS} ${HEADERS_PUBLIC})
target_include_directories(lunar PRIVATE src)
target_include_directories(lunar PUBLIC include)
target_link_libraries(lunar PRIVATE lunatic atom-math)
target_link_libraries(lunar PUBLIC atom-common atom-logger)

# TODO: remove this before merging the OpenGL renderer
find_package(SDL2 REQUIRED)
target_include_directories(lunar PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(lunar PRIVATE ${SDL2_LIBRARY})
