/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/bit.hpp>
#include <common/log.hpp>
#include <common/meta.hpp>
#include <fstream>

#include "bus.hpp"

namespace fauxDS::core {

ARM9MemoryBus::ARM9MemoryBus(Interconnect* interconnect)
    : ewram(interconnect->ewram)
    , swram(interconnect->swram.arm9)
    , ipc(interconnect->ipc)
    , irq9(interconnect->irq9)
    , video_unit(interconnect->video_unit)
    , vram(interconnect->video_unit.vram)
    , wramcnt(interconnect->wramcnt)
    , keyinput(interconnect->keyinput) {
  std::ifstream file { "bios9.bin", std::ios::in | std::ios::binary };
  ASSERT(file.good(), "ARM9: failed to open bios9.bin");
  file.read(reinterpret_cast<char*>(bios), 4096);
  ASSERT(file.good(), "ARM9: failed to read 4096 bytes from bios9.bin");
}

template <typename T>
auto ARM9MemoryBus::Read(u32 address, Bus bus) -> T {
  auto bitcount = bit::number_of_bits<T>();

  static_assert(common::is_one_of_v<T, u8, u16, u32>, "T must be u8, u16 or u32"); 

  if (itcm_config.enable_read && address >= itcm_config.base && address <= itcm_config.limit) {
    return *reinterpret_cast<T*>(&itcm[(address - itcm_config.base) & 0x7FFF]);
  }

  if (dtcm_config.enable_read && bus == Bus::Data && address >= dtcm_config.base && address <= dtcm_config.limit) {
    return *reinterpret_cast<T*>(&dtcm[(address - dtcm_config.base) & 0x3FFF]);
  }

  switch (address >> 24) {
    case 0x02:
      return *reinterpret_cast<T*>(&ewram[address & 0x3FFFFF]);
    case 0x03:
      if (swram.data == nullptr) {
        LOG_ERROR("ARM9: attempted to read SWRAM but it isn't mapped.");
        return 0;
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
    case 0x05:
      return *reinterpret_cast<T*>(&video_unit.pram[address & 0x7FF]);
    case 0x06:
      switch ((address >> 20) & 15) {
        /// PPU A - BG VRAM (max 512 KiB)
        case 0:
        case 1:
          return vram.region_ppu_a_bg.Read<T>(address & 0x1FFFFF);

        /// PPU B - BG VRAM (max 512 KiB)
        case 2:
        case 3:
          return vram.region_ppu_b_bg.Read<T>(address & 0x1FFFFF);

        /// PPU A - OBJ VRAM (max 256 KiB)
        case 4:
        case 5:
          return vram.region_ppu_a_obj.Read<T>(address & 0x1FFFFF);

        /// PPU B - OBJ VRAM (max 128 KiB)
        case 6:                                                                                                                                                          
        case 7:
          return vram.region_ppu_b_obj.Read<T>(address & 0x1FFFFF);

        /// LCDC (max 656 KiB)
        default:
          return vram.region_lcdc.Read<T>(address & 0xFFFFF);
      }
      return 0;
    case 0xFF:
      // TODO: clean up address decoding and figure out out-of-bounds reads.
      if ((address & 0xFFFF0000) == 0xFFFF0000)
        return *reinterpret_cast<T*>(&bios[address & 0x7FFF]);
    default:
      ASSERT(false, "ARM9: unhandled read{0} from 0x{1:08X}", bitcount, address);
  }

  return 0;
}

template<typename T>
void ARM9MemoryBus::Write(u32 address, T value) {
  auto bitcount = bit::number_of_bits<T>();

  static_assert(common::is_one_of_v<T, u8, u16, u32>, "T must be u8, u16 or u32"); 

  if (itcm_config.enable && address >= itcm_config.base && address <= itcm_config.limit) {
    *reinterpret_cast<T*>(&itcm[(address - itcm_config.base) & 0x7FFF]) = value;
    return;
  }

  if (dtcm_config.enable && address >= dtcm_config.base && address <= dtcm_config.limit) {
    *reinterpret_cast<T*>(&dtcm[(address - dtcm_config.base) & 0x3FFF]) = value;
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
    case 0x05:
      *reinterpret_cast<T*>(&video_unit.pram[address & 0x7FF]) = value;
      break;
    case 0x06:
      switch ((address >> 20) & 15) {
        /// PPU A - BG VRAM (max 512 KiB)
        case 0:
        case 1:
          vram.region_ppu_a_bg.Write<T>(address & 0x1FFFFF, value);
          break;

        /// PPU B - BG VRAM (max 512 KiB)
        case 2:
        case 3:
          vram.region_ppu_b_bg.Write<T>(address & 0x1FFFFF, value);
          break;

        /// PPU A - OBJ VRAM (max 256 KiB)
        case 4:
        case 5:
          vram.region_ppu_a_obj.Write<T>(address & 0x1FFFFF, value);
          break;

        /// PPU B - OBJ VRAM (max 128 KiB)
        case 6:
        case 7:
          vram.region_ppu_b_obj.Write<T>(address & 0x1FFFFF, value);
          break;

        /// LCDC (max 656 KiB)
        default:
          vram.region_lcdc.Write<T>(address & 0xFFFFF, value);
          break;
      }
      break;
    default:
      // TODO: remove this. this is only there to ignore trace enable/disable commands in rockwrestler.
      if (address == 0x08005500 && value == 0x08005500)
        break;
      ASSERT(false, "ARM9: unhandled write{0} 0x{1:08X} = 0x{2:08X}", bitcount, address, value);
  }
}

auto ARM9MemoryBus::ReadByte(u32 address, Bus bus) -> u8 {
  return Read<u8>(address, bus);
}
  
auto ARM9MemoryBus::ReadHalf(u32 address, Bus bus) -> u16 {
  return Read<u16>(address, bus);
}
  
auto ARM9MemoryBus::ReadWord(u32 address, Bus bus) -> u32 {
  return Read<u32>(address, bus);
}
  
void ARM9MemoryBus::WriteByte(u32 address, u8 value) {
  Write<u8>(address, value);
}
  
void ARM9MemoryBus::WriteHalf(u32 address, u16 value) {
  Write<u16>(address, value);
}

void ARM9MemoryBus::WriteWord(u32 address, u32 value) {
  Write<u32>(address, value);
}

} // namespace fauxDS::core
