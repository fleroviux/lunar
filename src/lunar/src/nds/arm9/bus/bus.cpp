/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunar/log.hpp>
#include <fstream>

#include "common/meta.hpp"
#include "common/punning.hpp"
#include "bus.hpp"
#include "buildconfig.hpp"

namespace lunar::nds {

ARM9MemoryBus::ARM9MemoryBus(Interconnect* interconnect)
    : ewram(interconnect->ewram)
    , swram(interconnect->swram)
    , cart(interconnect->cart)
    , ipc(interconnect->ipc)
    , irq9(interconnect->irq9)
    , math(interconnect->math)
    , timer(interconnect->timer9)
    , dma(interconnect->dma9)
    , video_unit(interconnect->video_unit)
    , vram(interconnect->video_unit.vram)
    , exmemcnt(interconnect->exmemcnt)
    , keypad(interconnect->keypad) {
  std::ifstream file { "bios9.bin", std::ios::in | std::ios::binary };
  ASSERT(file.good(), "ARM9: failed to open bios9.bin");
  file.read(reinterpret_cast<char*>(bios), 4096);
  ASSERT(file.good(), "ARM9: failed to read 4096 bytes from bios9.bin");

  itcm.data = &itcm_data[0];
  dtcm.data = &dtcm_data[0];
  itcm.mask = 0x7FFF;
  dtcm.mask = 0x3FFF;

  if constexpr (gEnableFastMemory) {
    pagetable = std::make_unique<std::array<u8*, 1048576>>();

    UpdateMemoryMap(0, 0x100000000ULL);

    swram.AddCallback([this]() {
      UpdateMemoryMap(0x03000000, 0x04000000);
    });
  }

  postflag = 0;
}

void ARM9MemoryBus::UpdateMemoryMap(u32 address_lo, u64 address_hi) {
  auto& table = *pagetable;

  for (u64 address = address_lo; address < address_hi; address += kPageMask + 1) {
    auto index = address >> kPageShift;

    switch (address >> 24) {
      case 0x02: {
        table[index] = &ewram[address & 0x3FFFFF];
        break;
      }
      case 0x03: {
        if (swram.arm9.data != nullptr) {
          table[index] = &swram.arm9.data[address & swram.arm9.mask];
        } else {
          table[index] = nullptr;
        }
        break;
      }
      case 0x06: {
        table[index] = VisitVRAMByAddress<GetUnsafePointerFunctor<u8>>(address);
        break;
      }
      case 0xFF: {
        // TODO: clean up address decoding and figure out out-of-bounds reads.
        if ((address & 0xFFFF0000) == 0xFFFF0000)
          table[index] = &bios[address & 0x7FFF];
        break;
      }
      default: {
        table[index] = nullptr;
        break;
      }
    }
  }
}

template <typename T>
auto ARM9MemoryBus::Read(u32 address, Bus bus) -> T {
  static_assert(is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64");

  if (itcm.config.enable_read &&
      bus != Bus::System &&
      address >= itcm.config.base &&
      address <= itcm.config.limit) {
    return read<T>(itcm_data, (address - itcm.config.base) & 0x7FFF);
  }

  if (dtcm.config.enable_read &&
      bus == Bus::Data &&
      address >= dtcm.config.base &&
      address <= dtcm.config.limit) {
    return read<T>(dtcm_data, (address - dtcm.config.base) & 0x3FFF);
  }

  switch (address >> 24) {
    case 0x02: {
      return read<T>(&ewram[0], address & 0x3FFFFF);
    }
    case 0x03: {
      if (swram.arm9.data == nullptr) {
        LOG_ERROR("ARM9: attempted to read SWRAM but it isn't mapped.");
        return 0;
      }
      return read<T>(swram.arm9.data, address & swram.arm9.mask);
    }
    case 0x04: {
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
    }
    case 0x05: {
      return read<T>(video_unit.pram, address & 0x7FF);
    }
    case 0x06: {
      return VisitVRAMByAddress<ReadFunctor<T>>(address);
    }
    case 0x07: {
      return read<T>(video_unit.oam, address & 0x7FF);
    }
    case 0x08: {
      return 0xFF;
    }
    case 0xFF: {
      // TODO: clean up address decoding and figure out out-of-bounds reads.
      if ((address & 0xFFFF0000) == 0xFFFF0000)
        return read<T>(bios, address & 0x7FFF);
    }
    default: {
      LOG_ERROR("ARM9: unhandled read{0} from 0x{1:08X}", sizeof(T) * 8, address);
    }
  }

  return 0;
}

template<typename T>
void ARM9MemoryBus::Write(u32 address, T value, Bus bus) {
  static_assert(is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64");

  if (bus != Bus::System) {
    if (itcm.config.enable &&
        address >= itcm.config.base &&
        address <= itcm.config.limit) {
      write<T>(itcm_data, (address - itcm.config.base) & 0x7FFF, value);
      return;
    }

    if (dtcm.config.enable &&
        address >= dtcm.config.base &&
        address <= dtcm.config.limit) {
      write<T>(dtcm_data, (address - dtcm.config.base) & 0x3FFF, value);
      return;
    }
  }

  switch (address >> 24) {
    case 0x02: {
      write<T>(&ewram[0], address & 0x3FFFFF, value);
      break;
    }
    case 0x03: {
      if (swram.arm9.data == nullptr) {
        LOG_ERROR("ARM9: attempted to read from SWRAM but it isn't mapped.");
        return;
      }
      write<T>(swram.arm9.data, address & swram.arm9.mask, value);
      break;
    }
    case 0x04: {
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
    }
    case 0x05: {
      u32 address_lo = address & 0x3FF;
      u32 address_hi = address_lo + sizeof(T);

      address &= 0x7FF;

      write<T>(video_unit.pram, address, value);

      if (address < 0x400) {
        video_unit.ppu_a.OnWritePRAM(address_lo, address_hi);
      } else {
        video_unit.ppu_b.OnWritePRAM(address_lo, address_hi);
      }
      break;
    }
    case 0x06: {
      VisitVRAMByAddress<WriteFunctor<T>>(address, value);

      // TODO: do this properly and remove template abuse.
      if (address < 0x06200000) {
        video_unit.ppu_a.OnWriteVRAM_BG(address & 0x1FFFFF, (address & 0x1FFFFF) + sizeof(T));
      } else if (address < 0x06400000) {
        video_unit.ppu_b.OnWriteVRAM_BG(address & 0x1FFFFF, (address & 0x1FFFFF) + sizeof(T));
      } else if (address < 0x06600000) {
        video_unit.ppu_a.OnWriteVRAM_OBJ(address & 0x1FFFFF, (address & 0x1FFFFF) + sizeof(T));
      } else if (address < 0x06800000) {
        video_unit.ppu_b.OnWriteVRAM_OBJ(address & 0x1FFFFF, (address & 0x1FFFFF) + sizeof(T));
      } else {
        video_unit.ppu_a.OnWriteVRAM_LCDC(address & 0xFFFFF, (address & 0xFFFFF) + sizeof(T));
      }
      break;
    }
    case 0x07: {
      u32 address_lo = address & 0x3FF;
      u32 address_hi = address_lo + sizeof(T);

      address &= 0x7FF;

      write<T>(video_unit.oam, address, value);

      if (address < 0x400) {
        video_unit.ppu_a.OnWriteOAM(address_lo, address_hi);
      } else {
        video_unit.ppu_b.OnWriteOAM(address_lo, address_hi);
      }
      break;
    }
    default: {
      LOG_ERROR("ARM9: unhandled write{0} 0x{1:08X} = 0x{2:08X}", sizeof(T) * 8, address, value);
    }
  }
}

template<class Functor, typename... Args>
auto ARM9MemoryBus::VisitVRAMByAddress(u32 address, Args... args) -> typename Functor::return_type {
  switch ((address >> 20) & 15) {
    // PPU A - BG VRAM (max 512 KiB)
    case 0 ... 1: {
      return (typename Functor::template value<32>){}(vram.region_ppu_bg[0], address & 0x1FFFFF, args...);
    }

    // PPU B - BG VRAM (max 512 KiB)
    case 2 ... 3: {
      return (typename Functor::template value<32>){}(vram.region_ppu_bg[1], address & 0x1FFFFF, args...);
    }
  
    // PPU A - OBJ VRAM (max 256 KiB)
    case 4 ... 5: {
      return (typename Functor::template value<16>){}(vram.region_ppu_obj[0], address & 0x1FFFFF, args...);
    }
  
    // PPU B - OBJ VRAM (max 128 KiB)
    case 6 ... 7: {
      return (typename Functor::template value<16>){}(vram.region_ppu_obj[1], address & 0x1FFFFF, args...);
    }
  
    // LCDC (max 656 KiB)
    default: {
      return (typename Functor::template value<64>){}(vram.region_lcdc, address & 0xFFFFF, args...);
    }
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

void ARM9MemoryBus::WriteByte(u32 address, u8 value, Bus bus) {
  Write<u8>(address, value, bus);
}

void ARM9MemoryBus::WriteHalf(u32 address, u16 value, Bus bus) {
  Write<u16>(address, value, bus);
}

void ARM9MemoryBus::WriteWord(u32 address, u32 value, Bus bus) {
  Write<u32>(address, value, bus);
}

} // namespace lunar::nds
