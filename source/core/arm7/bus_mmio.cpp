/*
 * Copyright (C) 2020 fleroviux
 */

#include "bus.hpp"

namespace fauxDS::core {

enum Registers {
};

auto ARM7MemoryBus::ReadByteIO(u32 address) -> u8 {
  return 0;
}

auto ARM7MemoryBus::ReadHalfIO(u32 address) -> u16 {
  return 0;
}

auto ARM7MemoryBus::ReadWordIO(u32 address) -> u32 {
  return 0;
}

void ARM7MemoryBus::WriteByteIO(u32 address,  u8 value) {
}

void ARM7MemoryBus::WriteHalfIO(u32 address, u16 value) {
}

void ARM7MemoryBus::WriteWordIO(u32 address, u32 value) {
}

} // namespace fauxDS::core
