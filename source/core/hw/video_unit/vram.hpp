/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <algorithm>
#include <common/integer.hpp>
#include <common/log.hpp>
#include <functional>
#include <memory>
#include <stddef.h>
#include <type_traits>
#include <vector>

namespace fauxDS::core {

struct VRAM {
  VRAM();

  void Reset();

  template<typename T>
  auto Read(u32 address) -> T {
    auto offset = address & 0x3FFF;

    switch (address) {
      /// PPU A - BG VRAM (max 512 KiB)
      case 0x06000000 ... 0x061FFFFF: {
        int page_id = ((address & 0x1FFFFF) >> 14) & 31;
        if (region_ppu_a_bg[page_id] != nullptr) {
          return *reinterpret_cast<T*>(&region_ppu_a_bg[page_id][offset]);
        }
        return 0;
      }

      /// PPU B - BG VRAM (max 128 KiB)
      case 0x06200000 ... 0x063FFFFF: {
        int page_id = ((address & 0x1FFFFF) >> 14) & 7;
        if (region_ppu_b_bg[page_id] != nullptr) {
          return *reinterpret_cast<T*>(&region_ppu_b_bg[page_id][offset]);
        }
        return 0;
      }

      /// PPU A - OBJ VRAM (max 256 KiB)
      case 0x06400000 ... 0x065FFFFF: {
        int page_id = ((address & 0x1FFFFF) >> 14) & 15;
        if (region_ppu_a_obj[page_id] != nullptr) {
          return *reinterpret_cast<T*>(&region_ppu_a_obj[page_id][offset]);
        }
        return 0;
      }

      /// PPU B - OBJ VRAM (max 128 KiB)
      case 0x06600000 ... 0x067FFFFF: {
        int page_id = ((address & 0x1FFFFF) >> 14) & 7;
        if (region_ppu_b_obj[page_id] != nullptr) {
          return *reinterpret_cast<T*>(&region_ppu_b_obj[page_id][offset]);
        }
        return 0;
      }

      /// LCDC (max 656 KiB)
      case 0x06800000 ... 0x06FFFFFF: {
        int page_id = ((address & 0xFFFFF) >> 14);
        // TODO: waste some memory and make this branchless?
        if (page_id < 41 && region_lcdc[page_id] != nullptr) {
          return *reinterpret_cast<T*>(&region_lcdc[page_id][offset]);
        }
        return 0;
      }
    }

    return 0;
  }
  
  template<typename T>
  void Write(u32 address, T value) {
    auto offset = address & 0x3FFF;

    switch (address) {
      /// PPU A - BG VRAM (max 512 KiB)
      case 0x06000000 ... 0x061FFFFF: {
        int page_id = ((address & 0x1FFFFF) >> 14) & 31;
        if (region_ppu_a_bg[page_id] != nullptr) {
          *reinterpret_cast<T*>(&region_ppu_a_bg[page_id][offset]) = value;
        }
        break;
      }

      /// PPU B - BG VRAM (max 128 KiB)
      case 0x06200000 ... 0x063FFFFF: {
        int page_id = ((address & 0x1FFFFF) >> 14) & 7;
        if (region_ppu_b_bg[page_id] != nullptr) {
          *reinterpret_cast<T*>(&region_ppu_b_bg[page_id][offset]) = value;
        }
        break;
      }

      /// PPU A - OBJ VRAM (max 256 KiB)
      case 0x06400000 ... 0x065FFFFF: {
        int page_id = ((address & 0x1FFFFF) >> 14) & 15;
        if (region_ppu_a_obj[page_id] != nullptr) {
          *reinterpret_cast<T*>(&region_ppu_a_obj[page_id][offset]) = value;
        }
        break;
      }

      /// PPU B - OBJ VRAM (max 128 KiB)
      case 0x06600000 ... 0x067FFFFF: {
        int page_id = ((address & 0x1FFFFF) >> 14) & 7;
        if (region_ppu_b_obj[page_id] != nullptr) {
          *reinterpret_cast<T*>(&region_ppu_b_obj[page_id][offset]) = value;
        }
        break;
      }

      /// LCDC (max 656 KiB)
      case 0x06800000 ... 0x06FFFFFF: {
        int page_id = ((address & 0xFFFFF) >> 14);
        // TODO: waste some memory and make this branchless?
        if (page_id < 41 && region_lcdc[page_id] != nullptr) {
          *reinterpret_cast<T*>(&region_lcdc[page_id][offset]) = value;
        }
        break;
      }
    }
  }

  template<typename T>
  auto Read7(u32 address) -> T {
    // TODO: confirm actual out-of-bounds behavior.
    address &= 0x3FFFF;

    switch (address) {
      case 0x00000 ... 0x1FFFF: {
        if (region_arm7[0] != nullptr) {
          return *reinterpret_cast<T*>(&region_arm7[0][address & 0x1FFFF]);
        }
        return 0;
      }
      case 0x20000 ... 0x3FFFF: {
        if (region_arm7[1] != nullptr) {
          return *reinterpret_cast<T*>(&region_arm7[1][address & 0x1FFFF]);
        }
        return 0;
      }
    }

    return 0;
  }

  template<typename T>
  void Write7(u32 address, T value) {
    // TODO: confirm actual out-of-bounds behavior.
    address &= 0x3FFFF;
    
    switch (address) {
      case 0x00000 ... 0x1FFFF: {
        if (region_arm7[0] != nullptr) {
          *reinterpret_cast<T*>(&region_arm7[0][address & 0x1FFFFF]) = value;
        }
        break;
      }
      case 0x20000 ... 0x3FFFF: {
        if (region_arm7[1] != nullptr) {
          *reinterpret_cast<T*>(&region_arm7[1][address & 0x1FFFFF]) = value;
        }
        break;
      }
    }
  }


  auto ReadVRAMSTAT() -> u8;
  void WriteVRAMCNT(int bank, u8 value);

private:
  /// LCDC and PPU A / B VRAM mapping (16 KiB pages)
  u8* region_ppu_a_bg[32] {nullptr};
  u8* region_ppu_b_bg[8] {nullptr};
  u8* region_ppu_a_obj[16] {nullptr};
  u8* region_ppu_b_obj[8] {nullptr};
  u8* region_lcdc[41] {nullptr};

  /// ARM7 bank C / D mapping
  u8* region_arm7[2] {nullptr};

  // TODO: fix this absolutely ugly madness.
  bool enable[9] { false };
  int mst[9] {0};
  int offset[9] {0};

  /// VRAM banks A-I (total: 656 KiB)
  u8 bank_a[0x20000]; // 128 KiB
  u8 bank_b[0x20000]; // 128 KiB
  u8 bank_c[0x20000]; // 128 KiB
  u8 bank_d[0x20000]; // 128 KiB
  u8 bank_e[0x10000]; //  64 KiB
  u8 bank_f[0x4000];  //  16 KiB
  u8 bank_g[0x4000];  //  16 KiB
  u8 bank_h[0x8000];  //  32 KiB
  u8 bank_i[0x4000];  //  16 KiB
};

} // namespace fauxDS::core
