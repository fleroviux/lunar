/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/bit.hpp>
#include <common/log.hpp>
#include <string.h>

#include "bus.hpp"

namespace fauxDS::core {

ARM7MemoryBus::ARM7MemoryBus(Interconnect* interconnect) 
    : ewram(interconnect->ewram)
    , swram(interconnect->swram.arm7)
    , ipc(interconnect->ipc)
    , irq7(interconnect->irq7)
    , video_unit(interconnect->video_unit)
    , keyinput(interconnect->keyinput) {
  memset(iwram, 0, sizeof(iwram));
}

template<typename T>
auto ARM7MemoryBus::Read(u32 address, Bus bus) -> T {
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
    case 0x04:
      if constexpr (std::is_same<T, u32>::value) {
        return ReadWordIO(address);
      }
      if constexpr (std::is_same<T, u16>::value) {
        return ReadHalfIO(address);
      }
      if constexpr (std::is_same<T, u8>::value) {
        return ReadByteIO(address);
      }
      return 0;
    default:
      ASSERT(false, "ARM7: unhandled read{0} from 0x{1:08X}", bitcount, address);
  }

  return 0;
}

template<typename T>
void ARM7MemoryBus::Write(u32 address, T value) {
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
    case 0x04:
      if constexpr (std::is_same<T, u32>::value) {
        WriteWordIO(address, value);
      }
      if constexpr (std::is_same<T, u16>::value) {
        WriteHalfIO(address, value);
      }
      if constexpr (std::is_same<T, u8>::value) {
        WriteByteIO(address, value);
      }
      break;
    default:
      ASSERT(false, "ARM7: unhandled write{0} 0x{1:08X} = 0x{2:02X}", bitcount, address, value);
  }
}

auto ARM7MemoryBus::ReadByte(u32 address, Bus bus) -> u8 {
  return Read<u8>(address, bus);
}
  
auto ARM7MemoryBus::ReadHalf(u32 address, Bus bus) -> u16 {
  return Read<u16>(address, bus);
}
  
auto ARM7MemoryBus::ReadWord(u32 address, Bus bus) -> u32 {
  return Read<u32>(address, bus);
}
  
void ARM7MemoryBus::WriteByte(u32 address, u8 value) {
  Write<u8>(address, value);
}
  
void ARM7MemoryBus::WriteHalf(u32 address, u16 value) {
  Write<u16>(address, value);
}

void ARM7MemoryBus::WriteWord(u32 address, u32 value) {
  Write<u32>(address, value);
}

} // namespace fauxDS::core
