/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/bit.hpp>
#include <common/log.hpp>
#include <fstream>
#include <string.h>

#include "bus.hpp"

namespace Duality::core {

ARM7MemoryBus::ARM7MemoryBus(Interconnect* interconnect) 
    : ewram(interconnect->ewram)
    , swram(interconnect->swram.arm7)
    , apu(interconnect->apu)
    , cart(interconnect->cart)
    , ipc(interconnect->ipc)
    , irq7(interconnect->irq7)
    , spi(interconnect->spi)
    , timer(interconnect->timer7)
    , dma(interconnect->dma7)
    , video_unit(interconnect->video_unit)
    , vram(interconnect->video_unit.vram)
    , wramcnt(interconnect->wramcnt)
    , keyinput(interconnect->keyinput)
    , extkeyinput(interconnect->extkeyinput) {
  std::ifstream file { "bios7.bin", std::ios::in | std::ios::binary };
  ASSERT(file.good(), "ARM7: failed to open bios7.bin");
  file.read(reinterpret_cast<char*>(bios), 16384);
  ASSERT(file.good(), "ARM7: failed to read 16384 bytes from bios7.bin");

  memset(iwram, 0, sizeof(iwram));
  apu.SetMemory(this);
  halted = false;

  if constexpr (gEnableFastMemory) {
    pagetable = std::make_unique<std::array<u8*, 1048576>>();
    UpdateMemoryMap(0, 0x100000000ULL);
    interconnect->wramcnt.AddCallback([this]() {
      UpdateMemoryMap(0x03000000, 0x04000000);
    });
  }
}

void ARM7MemoryBus::UpdateMemoryMap(u32 address_lo, u64 address_hi) {
  for (u64 address = address_lo; address < address_hi; address += kPageMask + 1) {
    switch (address >> 24) {
      case 0x00:
        (*pagetable)[address >> kPageShift] = &bios[address & 0x3FFF];
        break;
      case 0x02:
        (*pagetable)[address >> kPageShift] = &ewram[address & 0x3FFFFF];
        break;
      case 0x03:
        if ((address & 0x00800000) || swram.data == nullptr) {
          (*pagetable)[address >> kPageShift] = &iwram[address & 0xFFFF];
        } else {
          (*pagetable)[address >> kPageShift] = &swram.data[address & swram.mask];
        }
        break;
      default:
        (*pagetable)[address >> kPageShift] = nullptr;
        break;
    }
  }
}

template<typename T>
auto ARM7MemoryBus::Read(u32 address, Bus bus) -> T {
  auto bitcount = bit::number_of_bits<T>();

  static_assert(common::is_one_of_v<T, u8, u16, u32>, "T must be u8, u16 or u32"); 

  switch (address >> 24) {
    case 0x00:
      // TODO: figure out how out-of-bounds reads are supposed to work.
      return *reinterpret_cast<T*>(&bios[address & 0x3FFF]);
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
    case 0x06:
      return vram.region_arm7_wram.Read<T>(address);
    default:
      LOG_WARN("ARM7: unhandled read{0} from 0x{1:08X}", bitcount, address);
  }

  return 0;
}

template<typename T>
void ARM7MemoryBus::Write(u32 address, T value) {
  auto bitcount = bit::number_of_bits<T>();
  
  static_assert(common::is_one_of_v<T, u8, u16, u32>, "T must be u8, u16 or u32"); 

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
    case 0x06:
      vram.region_arm7_wram.Write<T>(address, value);
      break;
    default:
      LOG_WARN("ARM7: unhandled write{0} 0x{1:08X} = 0x{2:02X}", bitcount, address, value);
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

} // namespace Duality::core
