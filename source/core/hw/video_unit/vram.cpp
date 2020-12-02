/*
 * Copyright (C) 2020 fleroviux
 */

#include "vram.hpp"

namespace fauxDS::core {

VRAM::VRAM() {
  Reset();
}

void VRAM::Reset() {
  memset(bank_a.data(), 0, bank_a.size());
  memset(bank_b.data(), 0, bank_b.size());
  memset(bank_c.data(), 0, bank_c.size());
  memset(bank_d.data(), 0, bank_d.size());
  memset(bank_e.data(), 0, bank_e.size());
  memset(bank_f.data(), 0, bank_f.size());
  memset(bank_g.data(), 0, bank_g.size());
  memset(bank_h.data(), 0, bank_h.size());
  memset(bank_i.data(), 0, bank_i.size());

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
    vram.UnmapFromCurrent(bank);
  }

  mst = value & 7;
  offset = (value >> 3) & 3;
  enable = value & 0x80;

  if (enable) {
    vram.MapToCurrent(bank);
  }
}

auto VRAM::VRAMSTAT::ReadByte() -> u8 {
  return (vramcnt_c.enable && vramcnt_c.mst == 2 ? 1 : 0) |
         (vramcnt_d.enable && vramcnt_d.mst == 2 ? 2 : 0);
}

void VRAM::UnmapFromCurrent(Bank bank) {
  switch (bank) {
    case Bank::A:
      switch (vramcnt_a.mst) {
        case 0:
          region_lcdc.Unmap(0x00000, bank_a);
          break;
        case 1:
          region_ppu_bg[0].Unmap(0x20000 * vramcnt_a.offset, bank_a);
          break;
        case 2:
          region_ppu_obj[0].Unmap(0x20000 * (vramcnt_a.offset & 1), bank_a);
          break;
      }
      break;
    case Bank::B:
      switch (vramcnt_b.mst) {
        case 0:
          region_lcdc.Unmap(0x20000, bank_b);
          break;
        case 1:
          region_ppu_bg[0].Unmap(0x20000 * vramcnt_b.offset, bank_b);
          break;
        case 2:
          region_ppu_obj[0].Unmap(0x20000 * (vramcnt_b.offset & 1), bank_b);
          break;
      }
      break;
    case Bank::C:
      switch (vramcnt_c.mst) {
        case 0:
          region_lcdc.Unmap(0x40000, bank_c);
          break;
        case 1:
          region_ppu_bg[0].Unmap(0x20000 * vramcnt_c.offset, bank_c);
          break;
        case 2:
          region_arm7_wram.Unmap(0x20000 * (vramcnt_c.offset & 1), bank_c);
          break;
        case 4:
          region_ppu_bg[1].Unmap(0x00000, bank_c);
          break;
      }
      break;
    case Bank::D:
      switch (vramcnt_d.mst) {
        case 0:
          region_lcdc.Unmap(0x60000, bank_d);
          break;
        case 1:
          region_ppu_bg[0].Unmap(0x20000 * vramcnt_d.offset, bank_d);
          break;
        case 2:
          region_arm7_wram.Unmap(0x20000 * (vramcnt_d.offset & 1), bank_d);
          break;
        case 4:
          region_ppu_obj[1].Unmap(0x00000, bank_d);
          break;
      }
      break;
    case Bank::E:
      switch (vramcnt_e.mst) {
        case 0:
          region_lcdc.Unmap(0x80000, bank_e);
          break;
        case 1:
          region_ppu_bg[0].Unmap(0x00000, bank_e);
          break;
        case 2:
          region_ppu_obj[0].Unmap(0x00000, bank_e);
          break;
        case 4:
          region_ppu_bg_extpal[0].Unmap(0, bank_e, 0x8000);
          break;
      }
      break;
    case Bank::F:
      switch (vramcnt_f.mst) {
        case 0:
          region_lcdc.Unmap(0x90000, bank_f);
          break;
        case 1:
          region_ppu_bg[0].Unmap(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f);
          break;
        case 2:
          region_ppu_obj[0].Unmap(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f);
          break;
        case 4:
          region_ppu_bg_extpal[0].Unmap(0x4000 * (vramcnt_f.offset & 1), bank_f);
          break;
        case 5:
          region_ppu_obj_extpal[0].Unmap(0, bank_f, 0x2000);
          break;
      }
      break;
    case Bank::G:
      switch (vramcnt_g.mst) {
        case 0:
          region_lcdc.Unmap(0x94000, bank_g);
          break;
        case 1:
          region_ppu_bg[0].Unmap(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g);
          break;
        case 2:
          region_ppu_obj[0].Unmap(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g);
          break;
        case 4:
          region_ppu_bg_extpal[0].Unmap(0x4000 * (vramcnt_g.offset & 1), bank_g);
          break;
        case 5:
          region_ppu_obj_extpal[0].Unmap(0, bank_g, 0x2000);
          break;
      }
      break;
    case Bank::H:
      switch (vramcnt_h.mst) {
        case 0:
          region_lcdc.Unmap(0x98000, bank_h);
          break;
        case 1:
          region_ppu_bg[1].Unmap(0x00000, bank_h);
          break;
        case 2:
          region_ppu_bg_extpal[1].Unmap(0, bank_h);
          break;
      }
      break;
    case Bank::I:
      switch (vramcnt_i.mst) {
        case 0:
          region_lcdc.Unmap(0xA0000, bank_i);
          break;
        case 1:
          region_ppu_bg[1].Unmap(0x08000, bank_i);
          break;
        case 2:
          region_ppu_obj[1].Unmap(0x00000, bank_i);
          break;
        case 3:
          region_ppu_obj_extpal[1].Unmap(0, bank_i, 0x2000);
          break;
      }
      break;
  }
}

void VRAM::MapToCurrent(Bank bank) {
  switch (bank) {
    case Bank::A:
      switch (vramcnt_a.mst) {
        case 0:
          region_lcdc.Map(0x00000, bank_a);
          break;
        case 1:
          region_ppu_bg[0].Map(0x20000 * vramcnt_a.offset, bank_a);
          break;
        case 2:
          region_ppu_obj[0].Map(0x20000 * (vramcnt_a.offset & 1), bank_a);
          break;
        default:
          LOG_ERROR("VRAM bank A: unsupported configuration: mst={0} offset={1}", vramcnt_a.mst, vramcnt_a.offset);
      }
      break;
    case Bank::B:
      switch (vramcnt_b.mst) {
        case 0:
          region_lcdc.Map(0x20000, bank_b);
          break;
        case 1:
          region_ppu_bg[0].Map(0x20000 * vramcnt_b.offset, bank_b);
          break;
        case 2:
          region_ppu_obj[0].Map(0x20000 * (vramcnt_b.offset & 1), bank_b);
          break;
        default:
          LOG_ERROR("VRAM bank B: unsupported configuration: mst={0} offset={1}", vramcnt_b.mst, vramcnt_b.offset);
      }
      break;
    case Bank::C:
      switch (vramcnt_c.mst) {
        case 0:
          region_lcdc.Map(0x40000, bank_c);
          break;
        case 1:
          region_ppu_bg[0].Map(0x20000 * vramcnt_c.offset, bank_c);
          break;
        case 2:
          region_arm7_wram.Map(0x20000 * (vramcnt_c.offset & 1), bank_c);
          break;
        case 4:
          region_ppu_bg[1].Map(0x00000, bank_c);
          break;
        default:
          LOG_ERROR("VRAM bank C: unsupported configuration: mst={0} offset={1}", vramcnt_c.mst, vramcnt_c.offset);
      }
      break;
    case Bank::D:
      switch (vramcnt_d.mst) {
        case 0:
          region_lcdc.Map(0x60000, bank_d);
          break;
        case 1:
          region_ppu_bg[0].Map(0x20000 * vramcnt_d.offset, bank_d);
          break;
        case 2:
          region_arm7_wram.Map(0x20000 * (vramcnt_d.offset & 1), bank_d);
          break;
        case 4:
          region_ppu_obj[1].Map(0x00000, bank_d);
          break;
        default:
          LOG_ERROR("VRAM bank D: unsupported configuration: mst={0} offset={1}", vramcnt_d.mst, vramcnt_d.offset);
      }
      break;
    case Bank::E:
      switch (vramcnt_e.mst) {
        case 0:
          region_lcdc.Map(0x80000, bank_e);
          break;
        case 1:
          region_ppu_bg[0].Map(0x00000, bank_e);
          break;
        case 2:
          region_ppu_obj[0].Map(0x00000, bank_e);
          break;
        case 4:
          region_ppu_bg_extpal[0].Map(0, bank_e, 0x8000);
          break;
        default:
          LOG_ERROR("VRAM bank E: unsupported configuration: mst={0} offset={1}", vramcnt_e.mst, vramcnt_e.offset);
      }
      break;
    case Bank::F:
      switch (vramcnt_f.mst) {
        case 0:
          region_lcdc.Map(0x90000, bank_f);
          break;
        case 1:
          region_ppu_bg[0].Map(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f);
          break;
        case 2:
          region_ppu_obj[0].Map(0x4000 * (vramcnt_f.offset & 1) + 0x10000 * ((vramcnt_f.offset >> 1) & 1), bank_f);
          break;
        case 4:
          region_ppu_bg_extpal[0].Map(0x4000 * (vramcnt_f.offset & 1), bank_f);
          break;
        case 5:
          region_ppu_obj_extpal[0].Map(0, bank_f, 0x2000);
          break;
        default:
          LOG_ERROR("VRAM bank F: unsupported configuration: mst={0} offset={1}", vramcnt_f.mst, vramcnt_f.offset);
      }
      break;
    case Bank::G:
      switch (vramcnt_g.mst) {
        case 0:
          region_lcdc.Map(0x94000, bank_g);
          break;
        case 1:
          region_ppu_bg[0].Map(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g);
          break;
        case 2:
          region_ppu_obj[0].Map(0x4000 * (vramcnt_g.offset & 1) + 0x10000 * ((vramcnt_g.offset >> 1) & 1), bank_g);
          break;
        case 4:
          region_ppu_bg_extpal[0].Map(0x4000 * (vramcnt_g.offset & 1), bank_g);
          break;
        case 5:
          region_ppu_obj_extpal[0].Map(0, bank_g, 0x2000);
          break;
        default:
          LOG_ERROR("VRAM bank G: unsupported configuration: mst={0} offset={1}", vramcnt_g.mst, vramcnt_g.offset);
      }
      break;
    case Bank::H:
      switch (vramcnt_h.mst) {
        case 0:
          region_lcdc.Map(0x98000, bank_h);
          break;
        case 1:
          region_ppu_bg[1].Map(0x00000, bank_h);
          break;
        case 2:
          region_ppu_bg_extpal[1].Map(0, bank_h);
          break;
        default:
          LOG_ERROR("VRAM bank H: unsupported configuration: mst={0} offset={1}", vramcnt_h.mst, vramcnt_h.offset);
      }
      break;
    case Bank::I:
      switch (vramcnt_i.mst) {
        case 0:
          region_lcdc.Map(0xA0000, bank_i);
          break;
        case 1:
          region_ppu_bg[1].Map(0x08000, bank_i);
          break;
        case 2:
          region_ppu_obj[1].Map(0x00000, bank_i);
          break;
        case 3:
          region_ppu_obj_extpal[1].Map(0, bank_i, 0x2000);
          break;
        default:
          LOG_ERROR("VRAM bank I: unsupported configuration: mst={0} offset={1}", vramcnt_i.mst, vramcnt_i.offset);
      }
      break;
  }
}

} // namespace fauxDS::core
