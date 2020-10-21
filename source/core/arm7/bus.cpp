/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/bit.hpp>
#include <common/log.hpp>
#include <string.h>

#include "bus.hpp"

namespace fauxDS::core {

ARM7MemoryBus::ARM7MemoryBus(Interconnect* interconnect) 
    : ewram(interconnect->ewram), swram(interconnect->swram.arm7) {
  memset(iwram, 0, sizeof(iwram));
}

template<typename T>
auto ARM7MemoryBus::Read(u32 address, Bus bus, int core) -> T {
  auto bitcount = bit::number_of_bits<T>();

  // TODO: fix this god-damn-fucking-awful formatting.
  static_assert(
    std::is_same<T, u32>::value ||
    std::is_same<T, u16>::value ||
    std::is_same<T, u8>::value, "T must be u32, u16 or u8");

  switch (address >> 24) {
    case 0x02:
      return *reinterpret_cast<T*>(&ewram[address & 0x3FFFFF]);
    case 0x03:
      if ((address & 0x00800000) || swram.data == nullptr) {
        return *reinterpret_cast<T*>(&iwram[address & 0xFFFF]);
      }
      return *reinterpret_cast<T*>(&swram.data[address & swram.mask]);
    default:
      ASSERT(false, "ARM7: unhandled read{0} from 0x{1:08X}", bitcount, address);
  }

  return 0;
}

template<typename T>
void ARM7MemoryBus::Write(u32 address, T value, int core) {
  auto bitcount = bit::number_of_bits<T>();
  
  // TODO: fix this god-damn-fucking-awful formatting.
  static_assert(
    std::is_same<T, u32>::value ||
    std::is_same<T, u16>::value ||
    std::is_same<T, u8>::value, "T must be u32, u16 or u8");

  switch (address >> 24) {
    case 0x02:
      *reinterpret_cast<T*>(&ewram[address & 0x3FFFFF]) = value;
      break;
    case 0x03:
      if ((address & 0x00800000) || swram.data == nullptr) {
        *reinterpret_cast<T*>(&iwram[address & 0xFFFF]) = value;
        break;
      }
      *reinterpret_cast<T*>(&swram.data[address & swram.mask]) = value;
      break;
    default:
      ASSERT(false, "ARM7: unhandled write{0} 0x{1:08X} = 0x{2:02X}", bitcount, address, value);
  }
}

auto ARM7MemoryBus::ReadByte(u32 address, Bus bus, int core) -> u8 {
  return Read<u8>(address, bus, core);
}
  
auto ARM7MemoryBus::ReadHalf(u32 address, Bus bus, int core) -> u16 {
  return Read<u16>(address, bus, core);
}
  
auto ARM7MemoryBus::ReadWord(u32 address, Bus bus, int core) -> u32 {
  return Read<u32>(address, bus, core);
}
  
void ARM7MemoryBus::WriteByte(u32 address, u8 value, int core) {
  Write<u8>(address, value, core);
}
  
void ARM7MemoryBus::WriteHalf(u32 address, u16 value, int core) {
  Write<u16>(address, value, core);
}

void ARM7MemoryBus::WriteWord(u32 address, u32 value, int core) {
  Write<u32>(address, value, core);
}

} // namespace fauxDS::core
