/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "bus.hpp"

namespace fauxDS::core {

ARM7MemoryBus::ARM7MemoryBus(Interconnect* interconnect) : interconnect(interconnect) {
}

auto ARM7MemoryBus::ReadByte(u32 address, Bus bus, int core) -> u8 {
  LOG_ERROR("ARM7: unhandled read byte from 0x{0:08X}", address);
  return 0;
}
  
auto ARM7MemoryBus::ReadHalf(u32 address, Bus bus, int core) -> u16 {
  return ReadByte(address + 0, bus, core) | 
        (ReadByte(address + 1, bus, core) << 8);
}
  
auto ARM7MemoryBus::ReadWord(u32 address, Bus bus, int core) -> u32 {
  return ReadHalf(address + 0, bus, core) | 
        (ReadHalf(address + 2, bus, core) << 16);
}
  
void ARM7MemoryBus::WriteByte(u32 address, u8 value, int core) {
  LOG_ERROR("ARM7: unhandled write byte 0x{0:08X} = 0x{1:02X}", address, value);
}
  
void ARM7MemoryBus::WriteHalf(u32 address, u16 value, int core) {
  WriteByte(address + 0, value & 0xFF, core);
  WriteByte(address + 1, value >> 8, core);
}

void ARM7MemoryBus::WriteWord(u32 address, u32 value, int core) {
  WriteHalf(address + 0, value & 0xFFFF, core);
  WriteHalf(address + 2, value >> 16, core);
}

} // namespace fauxDS::core
