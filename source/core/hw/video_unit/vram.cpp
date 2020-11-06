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
  bank_a = {};
  bank_b = {};
  bank_c = {};
  bank_d = {};
  bank_e = {};
  bank_f = {};
  bank_g = {};
  bank_h = {};
  bank_i = {};

  vramcnt_a.WriteByte(0);
  vramcnt_b.WriteByte(0);
  vramcnt_c.WriteByte(0);
  vramcnt_d.WriteByte(0);
  vramcnt_e.WriteByte(0);
  vramcnt_f.WriteByte(0);
  vramcnt_g.WriteByte(0);
  vramcnt_h.WriteByte(0);
  vramcnt_i.WriteByte(0);
}

void VRAM::VRAMCNT::WriteByte(u8 value) {
  if (enable) {
    vram.UnmapBank(bank);
  }

  mst = value & 7;
  offset = (value >> 3) & 3;
  enable = value & 0x80;

  if (enable) {
    vram.MapBank(bank);
  }
}

auto VRAM::VRAMSTAT::ReadByte() -> u8 {
  return (vramcnt_c.enable && vramcnt_c.mst == 2 ? 1 : 0) |
         (vramcnt_d.enable && vramcnt_d.mst == 2 ? 2 : 0);
}

void VRAM::UnmapBank(Bank bank) {
  switch (bank) {
    case Bank::A:
      switch (vramcnt_a.mst) {
        case 0:
          region_lcdc.Unmap(0x00000, bank_a);
          break;
        case 1:
          region_ppu_a_bg.Unmap(0x20000 * vramcnt_a.offset, bank_a);
          break;
        case 2:
          region_ppu_a_obj.Unmap(0x20000 * (vramcnt_a.offset & 1), bank_a);
          break;
      }
      break;
    case Bank::B:
      switch (vramcnt_b.mst) {
        case 0:
          region_lcdc.Unmap(0x20000, bank_b);
          break;
        case 1:
          region_ppu_a_bg.Unmap(0x20000 * vramcnt_b.offset, bank_b);
          break;
        case 2:
          region_ppu_a_obj.Unmap(0x20000 * (vramcnt_b.offset & 1), bank_b);
          break;
      }
      break;
    case Bank::C:
      switch (vramcnt_c.mst) {
        case 0:
          region_lcdc.Unmap(0x40000, bank_c);
          break;
        case 1:
          region_ppu_a_bg.Unmap(0x20000 * vramcnt_c.offset, bank_c);
          break;
        case 2:
          region_arm7_wram.Unmap(0x20000 * (vramcnt_c.offset & 1), bank_c);
          break;
        case 4:
          region_ppu_b_bg.Unmap(0x00000, bank_c);
          break;
      }
      break;
    case Bank::D:
      switch (vramcnt_d.mst) {
        case 0:
          region_lcdc.Unmap(0x60000, bank_d);
          break;
        case 1:
          region_ppu_a_bg.Unmap(0x20000 * vramcnt_d.offset, bank_d);
          break;
        case 2:
          region_arm7_wram.Unmap(0x20000 * (vramcnt_d.offset & 1), bank_d);
          break;
        case 4:
          region_ppu_b_obj.Unmap(0x00000, bank_d);
          break;
      }
      break;
    case Bank::E:
      switch (vramcnt_e.mst) {
        case 0:
          region_lcdc.Unmap(0x80000, bank_e);
          break;
        case 1:
          region_ppu_a_bg.Unmap(0x00000, bank_e);
          break;
        case 2:
          region_ppu_a_obj.Unmap(0x00000, bank_e);
          break;
      }
      break;
    case Bank::F:
      switch (vramcnt_f.mst) {
        case 0:
          region_lcdc.Unmap(0x90000, bank_f);
          break;
        case 1:
          region_ppu_a_bg.Unmap(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f);
          break;
        case 2:
          region_ppu_a_obj.Unmap(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f);
          break;
      }
      break;
    case Bank::G:
      switch (vramcnt_g.mst) {
        case 0:
          region_lcdc.Unmap(0x94000, bank_g);
          break;
        case 1:
          region_ppu_a_bg.Unmap(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g);
          break;
        case 2:
          region_ppu_a_obj.Unmap(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g);
          break;
      }
      break;
    case Bank::H:
      switch (vramcnt_h.mst) {
        case 0:
          region_lcdc.Unmap(0x98000, bank_h);
          break;
        case 1:
          region_ppu_b_bg.Unmap(0x00000, bank_h);
          break;
      }
      break;
    case Bank::I:
      switch (vramcnt_i.mst) {
        case 0:
          region_lcdc.Unmap(0xA0000, bank_i);
          break;
        case 1:
          region_ppu_b_bg.Unmap(0x08000, bank_i);
          break;
        case 2:
          region_ppu_b_obj.Unmap(0x00000, bank_i);
          break;
      }
      break;
  }
}

void VRAM::MapBank(Bank bank) {
  switch (bank) {
    case Bank::A:
      switch (vramcnt_a.mst) {
        case 0:
          region_lcdc.Map(0x00000, bank_a);
          break;
        case 1:
          region_ppu_a_bg.Map(0x20000 * vramcnt_a.offset, bank_a);
          break;
        case 2:
          region_ppu_a_obj.Map(0x20000 * (vramcnt_a.offset & 1), bank_a);
          break;
      }
      break;
    case Bank::B:
      switch (vramcnt_b.mst) {
        case 0:
          region_lcdc.Map(0x20000, bank_b);
          break;
        case 1:
          region_ppu_a_bg.Map(0x20000 * vramcnt_b.offset, bank_b);
          break;
        case 2:
          region_ppu_a_obj.Map(0x20000 * (vramcnt_b.offset & 1), bank_b);
          break;
      }
      break;
    case Bank::C:
      switch (vramcnt_c.mst) {
        case 0:
          region_lcdc.Map(0x40000, bank_c);
          break;
        case 1:
          region_ppu_a_bg.Map(0x20000 * vramcnt_c.offset, bank_c);
          break;
        case 2:
          region_arm7_wram.Map(0x20000 * (vramcnt_c.offset & 1), bank_c);
          break;
        case 4:
          region_ppu_b_bg.Map(0x00000, bank_c);
          break;
      }
      break;
    case Bank::D:
      switch (vramcnt_d.mst) {
        case 0:
          region_lcdc.Map(0x60000, bank_d);
          break;
        case 1:
          region_ppu_a_bg.Map(0x20000 * vramcnt_d.offset, bank_d);
          break;
        case 2:
          region_arm7_wram.Map(0x20000 * (vramcnt_d.offset & 1), bank_d);
          break;
        case 4:
          region_ppu_b_obj.Map(0x00000, bank_d);
          break;
      }
      break;
    case Bank::E:
      switch (vramcnt_e.mst) {
        case 0:
          region_lcdc.Map(0x80000, bank_e);
          break;
        case 1:
          region_ppu_a_bg.Map(0x00000, bank_e);
          break;
        case 2:
          region_ppu_a_obj.Map(0x00000, bank_e);
          break;
      }
      break;
    case Bank::F:
      switch (vramcnt_f.mst) {
        case 0:
          region_lcdc.Map(0x90000, bank_f);
          break;
        case 1:
          region_ppu_a_bg.Map(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f);
          break;
        case 2:
          region_ppu_a_obj.Map(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f);
          break;
      }
      break;
    case Bank::G:
      switch (vramcnt_g.mst) {
        case 0:
          region_lcdc.Map(0x94000, bank_g);
          break;
        case 1:
          region_ppu_a_bg.Map(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g);
          break;
        case 2:
          region_ppu_a_obj.Map(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g);
          break;
      }
      break;
    case Bank::H:
      switch (vramcnt_h.mst) {
        case 0:
          region_lcdc.Map(0x98000, bank_h);
          break;
        case 1:
          region_ppu_b_bg.Map(0x00000, bank_h);
          break;
      }
      break;
    case Bank::I:
      switch (vramcnt_i.mst) {
        case 0:
          region_lcdc.Map(0xA0000, bank_i);
          break;
        case 1:
          region_ppu_b_bg.Map(0x08000, bank_i);
          break;
        case 2:
          region_ppu_b_obj.Map(0x00000, bank_i);
          break;
      }
      break;
  }
}

} // namespace fauxDS::core
