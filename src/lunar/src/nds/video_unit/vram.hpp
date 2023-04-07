/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/integer.hpp>

#include "common/likely.hpp"
#include "vram_region.hpp"

namespace lunar::nds {

// Holds VRAM bank data and takes care of allocating VRAM banks to regions (uses).
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

  // VRAM banks A - I (total: 656 KiB)
  std::array<u8, 0x20000> bank_a; // 128 KiB
  std::array<u8, 0x20000> bank_b; // 128 KiB
  std::array<u8, 0x20000> bank_c; // 128 KiB
  std::array<u8, 0x20000> bank_d; // 128 KiB
  std::array<u8, 0x10000> bank_e; //  64 KiB
  std::array<u8,  0x4000> bank_f; //  16 KiB
  std::array<u8,  0x4000> bank_g; //  16 KiB
  std::array<u8,  0x8000> bank_h; //  32 KiB
  std::array<u8,  0x4000> bank_i; //  16 KiB

  // LCDC and PPU A / B VRAM mapping
  Region<32> region_ppu_bg [2] { 31, 7 };
  Region<16> region_ppu_obj[2] { 15, 7 };

  // ARM9 direct access "LCDC" mapping
  Region<64> region_lcdc { 63 };

  // ARM7 bank C / D mapping
  Region<16> region_arm7_wram { 15 };

  // PPU A / B extended palette slots
  Region<4, 8192> region_ppu_bg_extpal [2] { 3, 3 };
  Region<1, 8192> region_ppu_obj_extpal[2] { 0, 0 };

  // GPU texture and texture palette data
  Region<4, 131072> region_gpu_texture { 3 };
  Region<8> region_gpu_palette { 7 };

  // VRAM bank control register
  struct VRAMCNT {
    VRAMCNT(VRAM& vram, Bank bank) : vram(vram), bank(bank) {}

    void WriteByte(u8 value);
  private:
    friend struct VRAM;

    // Denotes the target address space / VRAM usage.
    // I'm not sure why GBATEK calls it "mst".
    int mst = 0;

    // Where to map the bank in the target address space.
    int offset = 0;

    // Is the VRAM bank mapped at all?
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

  // VRAM status register.
  // Denotes whether bank C and D are mapped to the ARM7 core.
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

  // Unmap VRAM bank from where it is currently mapped.
  void UnmapFromCurrent(Bank bank);

  // Map VRAM bank according to its current configuration.
  void MapToCurrent(Bank bank);
};

} // namespace lunar::nds
