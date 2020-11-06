/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/likely.hpp>

#include "vram_region.hpp"

namespace fauxDS::core {

struct VRAM {
  enum class Bank {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
    F = 5,
    G = 6,
    H = 7,
    I = 8
  };

  VRAM();

  void Reset();

  template<typename T>
  auto Read(u32 address) -> T {
    auto offset = address & 0x3FFF;

    switch (address) {
      /// PPU A - BG VRAM (max 512 KiB)
      case 0x06000000 ... 0x061FFFFF: {
        return region_ppu_a_bg.Read<T>(address & 0x1FFFFF);
      }

      /// PPU B - BG VRAM (max 128 KiB)
      case 0x06200000 ... 0x063FFFFF: {
        return region_ppu_b_bg.Read<T>(address & 0x1FFFFF);
      }

      /// PPU A - OBJ VRAM (max 256 KiB)
      case 0x06400000 ... 0x065FFFFF: {
        return region_ppu_a_obj.Read<T>(address & 0x1FFFFF);
      }

      /// PPU B - OBJ VRAM (max 128 KiB)
      case 0x06600000 ... 0x067FFFFF: {
        return region_ppu_b_obj.Read<T>(address & 0x1FFFFF);
      }

      /// LCDC (max 656 KiB)
      case 0x06800000 ... 0x06FFFFFF: {
        return region_lcdc.Read<T>(address & 0xFFFFF);
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
        region_ppu_a_bg.Write<T>(address & 0x1FFFFF, value);
        break;
      }

      /// PPU B - BG VRAM (max 128 KiB)
      case 0x06200000 ... 0x063FFFFF: {
        region_ppu_b_bg.Write<T>(address & 0x1FFFFF, value);
        break;
      }

      /// PPU A - OBJ VRAM (max 256 KiB)
      case 0x06400000 ... 0x065FFFFF: {
        region_ppu_a_obj.Write<T>(address & 0x1FFFFF, value);
        break;
      }

      /// PPU B - OBJ VRAM (max 128 KiB)
      case 0x06600000 ... 0x067FFFFF: {
        region_ppu_b_obj.Write<T>(address & 0x1FFFFF, value);
        break;
      }

      /// LCDC (max 656 KiB)
      case 0x06800000 ... 0x06FFFFFF: {
        region_lcdc.Write<T>(address & 0xFFFFF, value);
        break;
      }
    }
  }

  template<typename T>
  auto Read7(u32 address) -> T {
    return region_arm7_wram.Read<T>(address);
  }

  template<typename T>
  void Write7(u32 address, T value) {
    region_arm7_wram.Write<T>(address, value);
  }

  struct VRAMCNT {
    VRAMCNT(VRAM& vram, Bank bank) : vram(vram), bank(bank) {}

    void WriteByte(u8 value);
  private:
    friend struct VRAM;

    int mst = 0;
    int offset = 0;
    bool enable = false;

    VRAM& vram;
    Bank bank;
  };

  VRAMCNT vramcnt_a { *this, Bank::A };
  VRAMCNT vramcnt_b { *this, Bank::B };
  VRAMCNT vramcnt_c { *this, Bank::C };
  VRAMCNT vramcnt_d { *this, Bank::D };
  VRAMCNT vramcnt_e { *this, Bank::E };
  VRAMCNT vramcnt_f { *this, Bank::F };
  VRAMCNT vramcnt_g { *this, Bank::G };
  VRAMCNT vramcnt_h { *this, Bank::H };
  VRAMCNT vramcnt_i { *this, Bank::I };

  struct VRAMSTAT {
    VRAMSTAT(VRAMCNT const& vramcnt_c, VRAMCNT const& vramcnt_d)
        : vramcnt_c(vramcnt_c), vramcnt_d(vramcnt_d) {}

    auto ReadByte() -> u8;
  private:
    VRAMCNT const& vramcnt_c;
    VRAMCNT const& vramcnt_d;
  } vramstat { vramcnt_c, vramcnt_d };

private:
  friend struct VRAMCNT;

  void UnmapBank(Bank bank);
  void MapBank(Bank bank);

  /// LCDC and PPU A / B VRAM mapping (16 KiB pages)
  Region<32> region_ppu_a_bg  { 31 };
  Region<32> region_ppu_b_bg  {  7 };
  Region<16> region_ppu_a_obj { 15 };
  Region<16> region_ppu_b_obj {  7 };
  Region<41> region_lcdc { 63 };

  /// ARM7 bank C / D mapping
  Region<16> region_arm7_wram { 15 };

  /// VRAM banks A - I (total: 656 KiB)
  std::array<u8, 0x20000> bank_a; // 128 KiB
  std::array<u8, 0x20000> bank_b; // 128 KiB
  std::array<u8, 0x20000> bank_c; // 128 KiB
  std::array<u8, 0x20000> bank_d; // 128 KiB
  std::array<u8, 0x10000> bank_e; //  64 KiB
  std::array<u8,  0x4000> bank_f; //  16 KiB
  std::array<u8,  0x4000> bank_g; //  16 KiB
  std::array<u8,  0x8000> bank_h; //  32 KiB
  std::array<u8,  0x4000> bank_i; //  16 KiB
};

} // namespace fauxDS::core
