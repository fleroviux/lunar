/*
 * Copyright (C) 2020 fleroviux
 */

#include "bus.hpp"

namespace fauxDS::core {

enum Registers {
};

auto ARM9MemoryBus::ReadByteIO(u32 address) -> u8 {
  return 0;
}

auto ARM9MemoryBus::ReadHalfIO(u32 address) -> u16 {
  return 0;
}

auto ARM9MemoryBus::ReadWordIO(u32 address) -> u32 {
  return 0;
}

void ARM9MemoryBus::WriteByteIO(u32 address,  u8 value) {
}

void ARM9MemoryBus::WriteHalfIO(u32 address, u16 value) {
}

void ARM9MemoryBus::WriteWordIO(u32 address, u32 value) {
}

} // namespace fauxDS::core
