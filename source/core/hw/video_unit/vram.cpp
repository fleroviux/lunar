/*
 * Copyright (C) 2020 fleroviux
 */

#include <string.h>

#include "vram.hpp"

namespace fauxDS::core {

VRAM::VRAM() {
  Reset();
}

void VRAM::Reset() {
  memset(bank_a, 0, sizeof(bank_a));
  memset(bank_b, 0, sizeof(bank_b));
  memset(bank_c, 0, sizeof(bank_c));
  memset(bank_d, 0, sizeof(bank_d));
  memset(bank_e, 0, sizeof(bank_e));
  memset(bank_f, 0, sizeof(bank_f));
  memset(bank_g, 0, sizeof(bank_g));
  memset(bank_h, 0, sizeof(bank_h));
  memset(bank_i, 0, sizeof(bank_i));

  // TODO: we assume that everything is mapped in LCDC mode initially.
  for (int i = 0; i < 9; i++)
    WriteVRAMCNT(i, 0);
}

auto VRAM::ReadVRAMSTAT() -> u8 {
  return (enable[2] && mst[2] == 2 ? 1 : 0) |
         (enable[3] && mst[3] == 2 ? 2 : 0);
}

void VRAM::WriteVRAMCNT(int bank, u8 value) {
  // unmap
  if (enable[bank]) {
    switch (bank) {
      case 0: // A
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 8; i++)
              region_lcdc[i] = nullptr;
            break;
          case 1:
            for (int i = 0; i < 8; i++)
              region_ppu_a_bg[8 * offset[bank] + i] = nullptr;
            break;
          case 2:
            for (int i = 0; i < 8; i++)
              region_ppu_a_obj[8 * (offset[bank] & 1) + i] = nullptr;
            break;
        }
        break;
      case 1: // B
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 8; i++)
              region_lcdc[8 + i] = nullptr;
            break;
          case 1:
            for (int i = 0; i < 8; i++)
              region_ppu_a_bg[8 * offset[bank] + i] = nullptr;
            break;
          case 2:
            for (int i = 0; i < 8; i++)
              region_ppu_a_obj[8 * (offset[bank] & 1) + i] = nullptr;
            break;
        }
        break;
      case 2: // C
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 8; i++)
              region_lcdc[16 + i] = nullptr;
            break;
          case 1:
            for (int i = 0; i < 8; i++)
              region_ppu_a_bg[8 * offset[bank] + i] = nullptr;
            break;
          case 2:
            region_arm7[offset[bank] & 1] = nullptr;
            break;
          case 4:
            for (int i = 0; i < 8; i++)
              region_ppu_b_bg[i] = nullptr;
            break;
        }
        break;
      case 3: // D
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 8; i++)
              region_lcdc[24 + i] = nullptr;
            break;
          case 1:
            for (int i = 0; i < 8; i++)
              region_ppu_a_bg[8 * offset[bank] + i] = nullptr;
            break;
          case 2:
            region_arm7[offset[bank] & 1] = nullptr;
            break;
          case 4:
            for (int i = 0; i < 8; i++)
              region_ppu_b_obj[i] = nullptr;
            break;
        }
        break;
      case 4: // E
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 4; i++)
              region_lcdc[32 + i] = nullptr;
            break;
          case 1:
            for (int i = 0; i < 4; i++)
              region_ppu_a_bg[i] = nullptr;
            break;
          case 2:
            for (int i = 0; i < 4; i++)
              region_ppu_a_obj[i] = nullptr;
            break;
        }
        break;
      case 5: // F
        switch (mst[bank]) {
          case 0:
            region_lcdc[36] = &bank_f[0];
            break;
          case 1:
            region_ppu_a_bg[(offset[bank] & 1) + ((offset[bank] >> 1) & 1) * 4] = nullptr;
            break;
          case 2:
            region_ppu_a_obj[(offset[bank] & 1) + ((offset[bank] >> 1) & 1) * 4] = nullptr;
            break;
        }
        break;
      case 6: // G
      switch (mst[bank]) {
          case 0:
            region_lcdc[37] = nullptr;
            break;
          case 1:
            region_ppu_a_bg[(offset[bank] & 1) + ((offset[bank] >> 1) & 1) * 4] = nullptr;
            break;
          case 2:
            region_ppu_a_obj[(offset[bank] & 1) + ((offset[bank] >> 1) & 1) * 4] = nullptr;
            break;
        }
        break;
      case 7: // H
        switch (mst[bank]) {
          case 0:
            region_lcdc[38] = nullptr;
            region_lcdc[39] = nullptr;
            break;
          case 1:
            region_ppu_b_bg[0] = nullptr;
            region_ppu_b_bg[1] = nullptr;
            break;
        }
        break;
      case 8: // I
        switch (mst[bank]) {
          case 0:
            region_lcdc[40] = nullptr;
            break;
          case 1:
            region_ppu_b_bg[2] = nullptr;
            break;
          case 2:
            region_ppu_b_obj[0] = nullptr;
            break;
        }
        break;
    }
  }

  mst[bank] = value & 7;
  offset[bank] = (value >> 3) & 3;
  enable[bank] = value & 0x80;

  // remap
  if (enable[bank]) {
    switch (bank) {
      case 0: // A
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 8; i++)
              region_lcdc[i] = &bank_a[i * 0x4000];
            break;
          case 1:
            for (int i = 0; i < 8; i++)
              region_ppu_a_bg[8 * offset[bank] + i] = &bank_a[i * 0x4000];
            break;
          case 2:
            for (int i = 0; i < 8; i++)
              region_ppu_a_obj[8 * (offset[bank] & 1) + i] = &bank_a[i * 0x4000];
            break;
          default:
            LOG_ERROR("VRAM: bank[{0}] unhandled MST={1}!", bank, mst[bank]);
        }
        break;
      case 1: // B
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 8; i++)
              region_lcdc[8 + i] = &bank_b[i * 0x4000];
            break;
          case 1:
            for (int i = 0; i < 8; i++)
              region_ppu_a_bg[8 * offset[bank] + i] = &bank_b[i * 0x4000];
            break;
          case 2:
            for (int i = 0; i < 8; i++)
              region_ppu_a_obj[8 * (offset[bank] & 1) + i] = &bank_b[i * 0x4000];
            break;
          default:
            LOG_ERROR("VRAM: bank[{0}] unhandled MST={1}!", bank, mst[bank]);
        }
        break;
      case 2: // C
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 8; i++)
              region_lcdc[16 + i] = &bank_c[i * 0x4000];
            break;
          case 1:
            for (int i = 0; i < 8; i++)
              region_ppu_a_bg[8 * offset[bank] + i] = &bank_c[i * 0x4000];
            break;
          case 2:
            region_arm7[offset[bank] & 1] = &bank_c[0];
            break;
          case 4:
            for (int i = 0; i < 8; i++)
              region_ppu_b_bg[i] = &bank_c[i * 0x4000];
            break;
          default:
            LOG_ERROR("VRAM: bank[{0}] unhandled MST={1}!", bank, mst[bank]);
        }
        break;
      case 3: // D
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 8; i++)
              region_lcdc[24 + i] = &bank_d[i * 0x4000];
            break;
          case 1:
            for (int i = 0; i < 8; i++)
              region_ppu_a_bg[8 * offset[bank] + i] = &bank_d[i * 0x4000];
            break;
          case 2:
            region_arm7[offset[bank] & 1] = &bank_d[0];
            break;
          case 4:
            for (int i = 0; i < 8; i++)
              region_ppu_b_obj[i] = &bank_d[i * 0x4000];
            break;
          default:
            LOG_ERROR("VRAM: bank[{0}] unhandled MST={1}!", bank, mst[bank]);
        }
        break;
      case 4: // E
        switch (mst[bank]) {
          case 0:
            for (int i = 0; i < 4; i++)
              region_lcdc[32 + i] = &bank_e[i * 0x4000];
            break;
          case 1:
            for (int i = 0; i < 4; i++)
              region_ppu_a_bg[i] = &bank_e[i * 0x4000];
            break;
          case 2:
            for (int i = 0; i < 4; i++)
              region_ppu_a_obj[i] = &bank_e[i * 0x4000];
            break;
          default:
            LOG_ERROR("VRAM: bank[{0}] unhandled MST={1}!", bank, mst[bank]);
        }
        break;
      case 5: // F
        switch (mst[bank]) {
          case 0:
            region_lcdc[36] = &bank_f[0];
            break;
          case 1:
            region_ppu_a_bg[(offset[bank] & 1) + ((offset[bank] >> 1) & 1) * 4] = &bank_f[0];
            break;
          case 2:
            region_ppu_a_obj[(offset[bank] & 1) + ((offset[bank] >> 1) & 1) * 4] = &bank_f[0];
            break;
          default:
            LOG_ERROR("VRAM: bank[{0}] unhandled MST={1}!", bank, mst[bank]);
        }
        break;
      case 6: // G
      switch (mst[bank]) {
          case 0:
            region_lcdc[37] = &bank_g[0];
            break;
          case 1:
            region_ppu_a_bg[(offset[bank] & 1) + ((offset[bank] >> 1) & 1) * 4] = &bank_g[0];
            break;
          case 2:
            region_ppu_a_obj[(offset[bank] & 1) + ((offset[bank] >> 1) & 1) * 4] = &bank_g[0];
            break;
          default:
            LOG_ERROR("VRAM: bank[{0}] unhandled MST={1}!", bank, mst[bank]);
        }
        break;
      case 7: // H
        switch (mst[bank]) {
          case 0:
            region_lcdc[38] = &bank_h[0x0000];
            region_lcdc[39] = &bank_h[0x4000];
            break;
          case 1:
            region_ppu_b_bg[0] = &bank_h[0x0000];
            region_ppu_b_bg[1] = &bank_h[0x4000];
            break;
          default:
            LOG_ERROR("VRAM: bank[{0}] unhandled MST={1}!", bank, mst[bank]);
        }
        break;
      case 8: // I
        switch (mst[bank]) {
          case 0:
            region_lcdc[40] = &bank_i[0];
            break;
          case 1:
            region_ppu_b_bg[2] = &bank_i[0];
            break;
          case 2:
            region_ppu_b_obj[0] = &bank_i[0];
            break;
        }
        break;
      default:
        LOG_ERROR("VRAM: bank[{0}] unhandled MST={1}!", bank, mst[bank]);
    }
  }
}

/*auto VRAM::GetBankAddress(Bank bank, int mst, int offset) -> u32 {
  // The most terrible code not so proudly presented to you by fleroviux.
  switch (bank) {
    case Bank::A:
      switch (mst) {
        case 0: return 0x06800000;
        case 1: return 0x06000000 + 0x20000 * offset;
        case 2: return 0x06400000 + 0x20000 * (offset & 1);
      }
      break;
    case Bank::B:
      switch (mst) {
        case 0: return 0x06820000;
        case 1: return 0x06000000 + 0x20000 * offset;
        case 2: return 0x06400000 + 0x20000 * (offset & 1);
      }
      break;
    case Bank::C:
     switch (mst) {
        case 0: return 0x06840000;
        case 1: return 0x06000000 + 0x20000 * offset;
        case 4: return 0x06200000;
      }
      break;
    case Bank::D:
      switch (mst) {
        case 0: return 0x06860000;
        case 1: return 0x06000000 + 0x20000 * offset;
        case 4: return 0x06600000;
      }
      break;
    case Bank::E:
      switch (mst) {
        case 0: return 0x06880000;
        case 1: return 0x06000000;
        case 2: return 0x06400000;
      }
      break;
    case Bank::F:
      switch (mst) {
        case 0: return 0x06890000;
        case 1: return 0x06000000 + 0x4000 * (offset & 1) + 0x10000 * ((offset >> 1) & 1);
        case 2: return 0x06400000 + 0x4000 * (offset & 1) + 0x10000 * ((offset >> 1) & 1);
      }
      break;
    case Bank::G:
      switch (mst) {
        case 0: return 0x06894000;
        case 1: return 0x06000000 + 0x4000 * (offset & 1) + 0x10000 * ((offset >> 1) & 1);
        case 2: return 0x06400000 + 0x4000 * (offset & 1) + 0x10000 * ((offset >> 1) & 1);
      }
      break;
    case Bank::H:
      switch (mst) {
        case 0: return 0x06898000;
        case 1: return 0x06200000;
      }
      break;
    case Bank::I:
      switch (mst) {
        case 0: return 0x068A0000;
        case 1: return 0x06208000;
        case 2: return 0x06600000;
      }
      break;
  }

  ASSERT(false, "VRAM: unhandled bank={0} mst={1} offset={2}.", bank, mst, offset);
}*/

} // namespace fauxDS::core
