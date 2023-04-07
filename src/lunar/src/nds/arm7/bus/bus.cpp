/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <atom/punning.hpp>
#include <atom/meta.hpp>
#include <lunar/log.hpp>
#include <fstream>
#include <string.h>

#include "bus.hpp"
#include "buildconfig.hpp"

namespace lunar::nds {

ARM7MemoryBus::ARM7MemoryBus(Interconnect* interconnect) 
    : ewram(interconnect->ewram)
    , swram(interconnect->swram)
    , apu(interconnect->apu)
    , cart(interconnect->cart)
    , ipc(interconnect->ipc)
    , irq7(interconnect->irq7)
    , spi(interconnect->spi)
    , timer(interconnect->timer7)
    , dma(interconnect->dma7)
    , video_unit(interconnect->video_unit)
    , vram(interconnect->video_unit.vram)
    , wifi(interconnect->wifi)
    , exmemcnt(interconnect->exmemcnt)
    , keypad(interconnect->keypad)
    , rtc(interconnect->rtc) {
  std::ifstream file { "bios7.bin", std::ios::in | std::ios::binary };
  ASSERT(file.good(), "ARM7: failed to open bios7.bin");
  file.read(reinterpret_cast<char*>(bios), 16384);
  ASSERT(file.good(), "ARM7: failed to read 16384 bytes from bios7.bin");

  memset(iwram, 0, sizeof(iwram));
  halted = false;
  postflag = 0;
  soundbias = 0x200;

  if constexpr (gEnableFastMemory) {
    pagetable = std::make_unique<std::array<u8*, 1048576>>();
    UpdateMemoryMap(0, 0x100000000ULL);
    swram.AddCallback([this]() {
      UpdateMemoryMap(0x03000000, 0x04000000);
    });
    vram.region_arm7_wram.AddCallback([this](u32 offset, size_t size) {
      UpdateMemoryMap(0x06000000, 0x06000000 + size);
    });
  }
}

void ARM7MemoryBus::UpdateMemoryMap(u32 address_lo, u64 address_hi) {
  auto& table = *pagetable;

  for (u64 address = address_lo; address < address_hi; address += kPageMask + 1) {
    auto index = address >> kPageShift;

    switch (address >> 24) {
      case 0x00: {
        table[index] = &bios[address & 0x3FFF];
        break;
      }
      case 0x02: {
        table[index] = &ewram[address & 0x3FFFFF];
        break;
      }
      case 0x03: {
        if ((address & 0x00800000) || swram.arm7.data == nullptr) {
          table[index] = &iwram[address & 0xFFFF];
        } else {
          table[index] = &swram.arm7.data[address & swram.arm7.mask];
        }
        break;
      }
      case 0x06: {
        table[index] = vram.region_arm7_wram.GetUnsafePointer<u8>(address);
        break;
      }
      default: {
        table[index] = nullptr;
        break;
      }
    }
  }
}

template<typename T>
auto ARM7MemoryBus::Read(u32 address) -> T {
  static_assert(atom::is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64");

  switch (address >> 24) {
    case 0x00: {
      // TODO: figure out how out-of-bounds reads are supposed to work.
      return atom::read<T>(bios, address & 0x3FFF);
    }
    case 0x02: {
      return atom::read<T>(&ewram[0], address & 0x3FFFFF);
    }
    case 0x03: {
      if ((address & 0x00800000) || swram.arm7.data == nullptr) {
        return atom::read<T>(iwram, address & 0xFFFF);
      }
      return atom::read<T>(swram.arm7.data, address & swram.arm7.mask);
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
    case 0x06: {
      return vram.region_arm7_wram.Read<T>(address);
    }
    default: {
      LOG_WARN("ARM7: unhandled read{0} from 0x{1:08X}", sizeof(T) * 8, address);
    }
  }

  return 0;
}

template<typename T>
void ARM7MemoryBus::Write(u32 address, T value) {
  static_assert(atom::is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64");

  switch (address >> 24) {
    case 0x02: {
      atom::write<T>(&ewram[0], address & 0x3FFFFF, value);
      break;
    }
    case 0x03: {
      if ((address & 0x00800000) || swram.arm7.data == nullptr) {
        atom::write<T>(iwram, address & 0xFFFF, value);
        break;
      }
      atom::write<T>(swram.arm7.data, address & swram.arm7.mask, value);
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
    case 0x06: {
      vram.region_arm7_wram.Write<T>(address, value);
      break;
    }
    default: {
      LOG_WARN("ARM7: unhandled write{0} 0x{1:08X} = 0x{2:02X}", sizeof(T) * 8, address, value);
    }
  }
}

auto ARM7MemoryBus::ReadByte(u32 address, Bus bus) -> u8 {
  return Read<u8>(address);
}
  
auto ARM7MemoryBus::ReadHalf(u32 address, Bus bus) -> u16 {
  return Read<u16>(address);
}
  
auto ARM7MemoryBus::ReadWord(u32 address, Bus bus) -> u32 {
  return Read<u32>(address);
}
 
void ARM7MemoryBus::WriteByte(u32 address, u8 value, Bus bus) {
  Write<u8>(address, value);
}
  
void ARM7MemoryBus::WriteHalf(u32 address, u16 value, Bus bus) {
  Write<u16>(address, value);
}

void ARM7MemoryBus::WriteWord(u32 address, u32 value, Bus bus) {
  Write<u32>(address, value);
}

} // namespace lunar::nds
