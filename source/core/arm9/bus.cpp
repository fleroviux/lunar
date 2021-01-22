/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/bit.hpp>
#include <common/log.hpp>
#include <common/meta.hpp>
#include <fstream>

#include "bus.hpp"

namespace Duality::core {

ARM9MemoryBus::ARM9MemoryBus(Interconnect* interconnect)
    : ewram(interconnect->ewram)
    , swram(interconnect->swram.arm9)
    , cart(interconnect->cart)
    , ipc(interconnect->ipc)
    , irq9(interconnect->irq9)
    , math_engine(interconnect->math_engine)
    , timer(interconnect->timer9)
    , dma(interconnect->dma9)
    , video_unit(interconnect->video_unit)
    , vram(interconnect->video_unit.vram)
    , wramcnt(interconnect->wramcnt)
    , keyinput(interconnect->keyinput) {
  std::ifstream file { "bios9.bin", std::ios::in | std::ios::binary };
  ASSERT(file.good(), "ARM9: failed to open bios9.bin");
  file.read(reinterpret_cast<char*>(bios), 4096);
  ASSERT(file.good(), "ARM9: failed to read 4096 bytes from bios9.bin");

  if constexpr (gEnableFastMemory) {
    pagetable = std::make_unique<std::array<u8*, 1048576>>();
    UpdateMemoryMap(0, 0x100000000ULL);
    interconnect->wramcnt.AddCallback([this]() {
      UpdateMemoryMap(0x03000000, 0x04000000);
    });
  }
}

void ARM9MemoryBus::SetDTCM(TCMConfig const& config) {
  if (dtcm_config.enable) {
    dtcm_config.enable = false;
    UpdateMemoryMap(dtcm_config.base, dtcm_config.limit + 1);
  }

  dtcm_config = config;

  if (dtcm_config.enable) {
    UpdateMemoryMap(dtcm_config.base, dtcm_config.limit + 1);
  }
}

void ARM9MemoryBus::SetITCM(TCMConfig const& config) {
  if (itcm_config.enable) {
    itcm_config.enable = false;
    UpdateMemoryMap(itcm_config.base, itcm_config.limit + 1);
  }

  itcm_config = config;

  if (itcm_config.enable) {
    UpdateMemoryMap(itcm_config.base, itcm_config.limit + 1);
  }
}

void ARM9MemoryBus::UpdateMemoryMap(u32 address_lo, u64 address_hi) {
  if constexpr (!gEnableFastMemory)
    return;

  for (u64 address = address_lo; address < address_hi; address += kPageMask + 1) {
    if (itcm_config.enable && address >= itcm_config.base && address <= itcm_config.limit) {
      (*pagetable)[address >> kPageShift] = nullptr;
      continue;
    }

    if (dtcm_config.enable && address >= dtcm_config.base && address <= dtcm_config.limit) {
      (*pagetable)[address >> kPageShift] = nullptr;
      continue;
    }

    switch (address >> 24) {
      case 0x02:
        (*pagetable)[address >> kPageShift] = &ewram[address & 0x3FFFFF];
        break;
      case 0x03:
        if (swram.data != nullptr) {
          (*pagetable)[address >> kPageShift] = &swram.data[address & swram.mask];
        } else {
          (*pagetable)[address >> kPageShift] = nullptr;
        }
        break;
      case 0x06:
        (*pagetable)[address >> kPageShift] = VisitVRAMByAddress<GetUnsafePointerFunctor<u8>>(address);
        break;
      case 0xFF:
        // TODO: clean up address decoding and figure out out-of-bounds reads.
        if ((address & 0xFFFF0000) == 0xFFFF0000)
          (*pagetable)[address >> kPageShift] = &bios[address & 0x7FFF];
        break;
      default:
        (*pagetable)[address >> kPageShift] = nullptr;
        break;
    }
  }
}

template <typename T>
auto ARM9MemoryBus::Read(u32 address, Bus bus) -> T {
  auto bitcount = bit::number_of_bits<T>();

  static_assert(common::is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64");

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
      if constexpr (std::is_same<T, u64>::value) {
        return ReadWordIO(address | 0) |
          (u64(ReadWordIO(address | 4)) << 32);
      }
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
      return VisitVRAMByAddress<ReadFunctor<T>>(address);
    case 0x07:
      return *reinterpret_cast<T*>(&video_unit.oam[address & 0x7FF]);
    case 0x08:
      return 0xFF;
    case 0xFF:
      // TODO: clean up address decoding and figure out out-of-bounds reads.
      if ((address & 0xFFFF0000) == 0xFFFF0000)
        return *reinterpret_cast<T*>(&bios[address & 0x7FFF]);
    default:
      LOG_ERROR("ARM9: unhandled read{0} from 0x{1:08X}", bitcount, address);
  }

  return 0;
}

template<typename T>
void ARM9MemoryBus::Write(u32 address, T value) {
  auto bitcount = bit::number_of_bits<T>();

  static_assert(common::is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64");

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
      if constexpr (std::is_same<T, u64>::value) {
        WriteWordIO(address | 0, value);
        WriteWordIO(address | 4, value >> 32);
      }
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
      VisitVRAMByAddress<WriteFunctor<T>>(address, value);
      break;
    case 0x07:
      *reinterpret_cast<T*>(&video_unit.oam[address & 0x7FF]) = value;
      break;
    default:
      LOG_ERROR("ARM9: unhandled write{0} 0x{1:08X} = 0x{2:08X}", bitcount, address, value);
  }
}

template<class Functor, typename... Args>
auto ARM9MemoryBus::VisitVRAMByAddress(u32 address, Args... args) -> typename Functor::return_type {
  switch ((address >> 20) & 15) {
    /// PPU A - BG VRAM (max 512 KiB)
    case 0 ... 1:
      return (typename Functor::template value<32>){}(vram.region_ppu_bg[0], address & 0x1FFFFF, args...);

    /// PPU B - BG VRAM (max 512 KiB)
    case 2 ... 3:
      return (typename Functor::template value<32>){}(vram.region_ppu_bg[1], address & 0x1FFFFF, args...);
  
    /// PPU A - OBJ VRAM (max 256 KiB)
    case 4 ... 5:
      return (typename Functor::template value<16>){}(vram.region_ppu_obj[0], address & 0x1FFFFF, args...);
  
    /// PPU B - OBJ VRAM (max 128 KiB)
    case 6 ... 7:
      return (typename Functor::template value<16>){}(vram.region_ppu_obj[1], address & 0x1FFFFF, args...);
  
    /// LCDC (max 656 KiB)
    default:
      return (typename Functor::template value<41>){}(vram.region_lcdc, address & 0xFFFFF, args...);
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

auto ARM9MemoryBus::ReadQuad(u32 address, Bus bus) -> u64 {
  return Read<u64>(address, bus);
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

void ARM9MemoryBus::WriteQuad(u32 address, u64 value) {
  Write<u64>(address, value);
}


} // namespace Duality::core
