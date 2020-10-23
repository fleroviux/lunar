/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "bus.hpp"

namespace fauxDS::core {

enum Registers {
};

auto ARM9MemoryBus::ReadByteIO(u32 address) -> u8 {
  switch (address) {

    default:
      LOG_WARN("ARM9: MMIO: unhandled read from 0x{0:08X}", address);
  }

  return 0;
}

auto ARM9MemoryBus::ReadHalfIO(u32 address) -> u16 {
  return (ReadByteIO(address | 0) << 0) |
         (ReadByteIO(address | 1) << 8);
}

auto ARM9MemoryBus::ReadWordIO(u32 address) -> u32 {
  return (ReadByteIO(address | 0) <<  0) |
         (ReadByteIO(address | 1) <<  8) |
         (ReadByteIO(address | 2) << 16) |
         (ReadByteIO(address | 3) << 24);
}

void ARM9MemoryBus::WriteByteIO(u32 address,  u8 value) {
  LOG_WARN("ARM9: MMIO: unhandled write to 0x{0:08X} = 0x{1:02X}", address, value);
}

void ARM9MemoryBus::WriteHalfIO(u32 address, u16 value) {
  WriteByteIO(address | 0, value & 0xFF);
  WriteByteIO(address | 1, value >> 8);
}

void ARM9MemoryBus::WriteWordIO(u32 address, u32 value) {
  WriteByteIO(address | 0, u8(value >>  0));
  WriteByteIO(address | 1, u8(value >>  8));
  WriteByteIO(address | 2, u8(value >> 16));
  WriteByteIO(address | 3, u8(value >> 24));
}

} // namespace fauxDS::core
