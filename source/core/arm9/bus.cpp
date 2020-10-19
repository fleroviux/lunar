/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/bit.hpp>
#include <common/log.hpp>
#include <type_traits>

#include "bus.hpp"

namespace fauxDS::core {

ARM9MemoryBus::ARM9MemoryBus(Interconnect* interconnect) : swram(interconnect->swram.arm9) {
  mmio.Map(0x0004, interconnect->fake_dispstat);
  mmio.Map(0x0130, interconnect->keyinput);
}

template <typename T>
auto ARM9MemoryBus::Read(u32 address, Bus bus, int core) -> T {
  auto bitcount = bit::number_of_bits<T>();

  // TODO: fix this god-damn-fucking-awful formatting.
  static_assert(
    std::is_same<T, u32>::value ||
    std::is_same<T, u16>::value ||
    std::is_same<T, u8>::value, "T must be u32, u16 or u8");

  if (address >= itcm_base && address <= itcm_limit)
    return *reinterpret_cast<T*>(&itcm[(address - itcm_base) & 0x7FFF]);

  if (bus == Bus::Data && address >= dtcm_base && address <= dtcm_limit)
    return *reinterpret_cast<T*>(&dtcm[(address - dtcm_base) & 0x3FFF]);

  switch (address >> 24) {
    // TODO: EWRAM actually is shared between the ARM9 and ARM7 core.
    case 0x02:
      return *reinterpret_cast<T*>(&ewram[address & 0x3FFFFF]);
    case 0x03:
      if (swram.data == nullptr) {
        LOG_ERROR("ARM9: attempted to read SWRAM but it isn't mapped.");
        return 0;
      }
      return *reinterpret_cast<T*>(&swram.data[address & swram.mask]);
    case 0x04:
      address &= 0x00FFFFFF;
      if constexpr (std::is_same<T, u32>::value) {
        return (mmio.Read(address | 0) <<  0) |
               (mmio.Read(address | 1) <<  8) |
               (mmio.Read(address | 2) << 16) |
               (mmio.Read(address | 3) << 24);
      }
      if constexpr (std::is_same<T, u16>::value) {
        return (mmio.Read(address | 0) << 0) |
               (mmio.Read(address | 1) << 8);
      }
      if constexpr (std::is_same<T, u8>::value) {
        return mmio.Read(address);
      }
    default:
      ASSERT(false, "ARM9: unhandled read{0} from 0x{1:08X}", bitcount, address);
  }

  return 0;
}


template<typename T>
void ARM9MemoryBus::Write(u32 address, T value, int core) {
  auto bitcount = bit::number_of_bits<T>();

  // TODO: fix this god-damn-fucking-awful formatting.
  static_assert(
    std::is_same<T, u32>::value ||
    std::is_same<T, u16>::value ||
    std::is_same<T, u8>::value, "T must be u32, u16 or u8");

  if (address >= itcm_base && address <= itcm_limit) {
    *reinterpret_cast<T*>(&itcm[(address - itcm_base) & 0x7FFF]) = value;
    return;
  }

  if (address >= dtcm_base && address <= dtcm_limit) {
    *reinterpret_cast<T*>(&dtcm[(address - dtcm_base) & 0x3FFF]) = value;
    return;
  }

  switch (address >> 24) {
    case 0x02:
      *reinterpret_cast<T*>(&ewram[address & 0x3FFFFF]) = value;
      break;
    case 0x03:
      if (swram.data == nullptr) {
        LOG_ERROR("ARM9: attempted to read from SWRAM but it isn't mapped.");
        return;
      }
      *reinterpret_cast<T*>(&swram.data[address & swram.mask]) = value;
      break;
    case 0x04:
      address &= 0x00FFFFFF;
      if constexpr (std::is_same<T, u32>::value) {
        mmio.Write(address | 0, (value >>  0) & 0xFF);
        mmio.Write(address | 1, (value >>  8) & 0xFF);
        mmio.Write(address | 2, (value >> 16) & 0xFF);
        mmio.Write(address | 3, (value >> 24) & 0xFF);
      }
      if constexpr (std::is_same<T, u16>::value) {
        mmio.Write(address | 0, value & 0xFF);
        mmio.Write(address | 1, value >> 8);
      }
      if constexpr (std::is_same<T, u8>::value) {
        mmio.Write(address, value & 0xFF);
      }
      break;
    case 0x06:
      *reinterpret_cast<T*>(&vram[address & 0x1FFFFF]) = value;
      break;
    default:
      ASSERT(false, "ARM9: unhandled write{0} 0x{1:08X} = 0x{2:08X}", bitcount, address, value);
  }
}

auto ARM9MemoryBus::ReadByte(u32 address, Bus bus, int core) -> u8 {
  return Read<u8>(address, bus, core);
}
  
auto ARM9MemoryBus::ReadHalf(u32 address, Bus bus, int core) -> u16 {
  return Read<u16>(address, bus, core);
}
  
auto ARM9MemoryBus::ReadWord(u32 address, Bus bus, int core) -> u32 {
  return Read<u32>(address, bus, core);
}
  
void ARM9MemoryBus::WriteByte(u32 address, u8 value, int core) {
  Write<u8>(address, value, core);
}
  
void ARM9MemoryBus::WriteHalf(u32 address, u16 value, int core) {
  Write<u16>(address, value, core);
}

void ARM9MemoryBus::WriteWord(u32 address, u32 value, int core) {
  Write<u32>(address, value, core);
}

} // namespace fauxDS::core
