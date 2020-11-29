/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>
#include <string.h>

#include "ppu.hpp"

namespace fauxDS::core {

PPU::PPU(int id, VRAM const& vram, u8 const* pram, u8 const* oam) 
    : id(id)
    , vram(vram)
    , vram_bg(vram.region_ppu_bg[id])
    , vram_obj(vram.region_ppu_obj[id])
    , pram(pram)
    , oam(oam) {
  if (id == 0) {
    mmio.dispcnt = {};
  } else {
    mmio.dispcnt = {0xC033FFF7};
  }
  Reset();
}

void PPU::Reset() {
  memset(framebuffer, 0, sizeof(framebuffer));

  mmio.dispcnt.Reset();
  
  for (int i = 0; i < 4; i++) {
    mmio.bgcnt[i].Reset();
    mmio.bghofs[i].Reset();
    mmio.bgvofs[i].Reset();
  }

  for (int i = 0; i < 2; i++) {
    mmio.bgpa[i].WriteByte(0, 0);
    mmio.bgpa[i].WriteByte(1, 1);
    mmio.bgpb[i].WriteByte(0, 0);
    mmio.bgpb[i].WriteByte(1, 0);
    mmio.bgpc[i].WriteByte(0, 0);
    mmio.bgpc[i].WriteByte(1, 0);
    mmio.bgpd[i].WriteByte(0, 0);
    mmio.bgpd[i].WriteByte(1, 1);
    mmio.bgx[i].Reset();
    mmio.bgy[i].Reset();
    mmio.winh[i].Reset();
    mmio.winv[i].Reset();
  }

  mmio.winin.Reset();
  mmio.winout.Reset();

  mmio.bldcnt.Reset();
  mmio.bldalpha.Reset();
  mmio.bldy.Reset();

  mmio.mosaic.Reset();
}

void PPU::OnDrawScanlineBegin(u16 vcount) {
  if (mmio.dispcnt.enable[ENABLE_WIN0]) {
    RenderWindow(0, vcount);
  }

  if (mmio.dispcnt.enable[ENABLE_WIN1]) {
    RenderWindow(1, vcount);
  }

  RenderScanline(vcount);
}

void PPU::OnDrawScanlineEnd() {
  auto& dispcnt = mmio.dispcnt;
  auto& bgx = mmio.bgx;
  auto& bgy = mmio.bgy;
  auto& mosaic = mmio.mosaic;

  // Advance vertical background mosaic counter
  if (++mosaic.bg._counter_y == mosaic.bg.size_y) {
    mosaic.bg._counter_y = 0;
  }

  // Advance vertical OBJ mosaic counter
  if (++mosaic.obj._counter_y == mosaic.obj.size_y) {
    mosaic.obj._counter_y = 0;
  }

  /* Mode 0 doesn't have any affine backgrounds,
   * in that case the internal registers seemingly aren't updated.
   * TODO: needs more research, e.g. what happens if all affine backgrounds are disabled?
   */
  if (dispcnt.bg_mode != 0) {
    // Advance internal affine registers and apply vertical mosaic if applicable.
    for (int i = 0; i < 2; i++) {
      if (mmio.bgcnt[2 + i].enable_mosaic) {
        if (mosaic.bg._counter_y == 0) {
          bgx[i]._current += mosaic.bg.size_y * mmio.bgpb[i].value;
          bgy[i]._current += mosaic.bg.size_y * mmio.bgpd[i].value;
        }
      } else {
        bgx[i]._current += mmio.bgpb[i].value;
        bgy[i]._current += mmio.bgpd[i].value;
      }
    }
  }
}

void PPU::OnBlankScanlineBegin(u16 vcount) {
  auto& bgx = mmio.bgx;
  auto& bgy = mmio.bgy;
  auto& mosaic = mmio.mosaic;

  // TODO: when exactly are these registers reloaded?
  if (vcount == 192) {
    // Reset vertical mosaic counters
    mosaic.bg._counter_y = 0;
    mosaic.obj._counter_y = 0;

    // Reload internal affine registers
    bgx[0]._current = bgx[0].initial;
    bgy[0]._current = bgy[0].initial;
    bgx[1]._current = bgx[1].initial;
    bgy[1]._current = bgy[1].initial;
  }

  if (mmio.dispcnt.enable[ENABLE_WIN0]) {
    RenderWindow(0, vcount);
  }

  if (mmio.dispcnt.enable[ENABLE_WIN1]) {
    RenderWindow(1, vcount);
  }
}

void PPU::RenderScanline(u16 vcount) {
  switch (mmio.dispcnt.display_mode) {
    case 0:
      RenderDisplayOff(vcount);
      break;
    case 1:
      RenderNormal(vcount);
      break;
    case 2:
      RenderVideoMemoryDisplay(vcount);
      break;
    case 3:
      RenderMainMemoryDisplay(vcount);
      break;
  }
}

void PPU::RenderDisplayOff(u16 vcount) {
  u32* line = &framebuffer[vcount * 256];

  for (uint x = 0; x < 256; x++) {
    line[x] = ConvertColor(0x7FFF);
  }
}

void PPU::RenderNormal(u16 vcount) {
  u32* line = &framebuffer[vcount * 256];

  if (mmio.dispcnt.forced_blank) {
    for (uint x = 0; x < 256; x++) {
      line[x] = ConvertColor(0x7FFF);
    }
    return;
  }

  // TODO: on a real Nintendo DS all sprites are rendered one scanline ahead.
  if (mmio.dispcnt.enable[ENABLE_OBJ]) {
    RenderLayerOAM(vcount);
  }

  switch (mmio.dispcnt.bg_mode) {
    // BG0 = Text/3D, BG1 - BG3 = Text
    case 0:
      for (uint i = 0; i < 4; i++) {
        if (mmio.dispcnt.enable[i]) {
          RenderLayerText(i, vcount);
        }
      }
      break;
    // BG0 = Text/3D, BG1 - BG2 = Text, BG = affine
    case 1:
      for (uint i = 0; i < 3; i++) {
        if (mmio.dispcnt.enable[i]) {
          RenderLayerText(i, vcount);
        }
      }
      if (mmio.dispcnt.enable[ENABLE_BG3]) {
        RenderLayerAffine(1);
      }
      break;
    // BG0 = Text/3D, BG1 = Text, BG2 - BG3 = affine
    case 2:
      for (uint i = 0; i < 2; i++) {
        if (mmio.dispcnt.enable[i]) {
          RenderLayerText(i, vcount);
        }
      }
      for (uint i = 0; i < 2; i++) {
        if (mmio.dispcnt.enable[2 + i]) {
          RenderLayerAffine(i);
        }
      }
      break;
    // BG0 = Text / 3D, BG1 - BG2 = Text, BG3 = extended
    case 3:
      for (uint i = 0; i < 3; i++) {
        if (mmio.dispcnt.enable[i]) {
          RenderLayerText(i, vcount);
        }
      }
      if (mmio.dispcnt.enable[ENABLE_BG3]) {
        RenderLayerExtended(1);
      }
      break;
    // BG0 = Text / 3D, BG1 = Text, BG2 = affine, BG3 = extended
    case 4:
      for (uint i = 0; i < 2; i++) {
        if (mmio.dispcnt.enable[i]) {
          RenderLayerText(i, vcount);
        }
      }
      if (mmio.dispcnt.enable[ENABLE_BG2]) {
        RenderLayerAffine(0);
      }
      if (mmio.dispcnt.enable[ENABLE_BG3]) {
        RenderLayerExtended(1);
      }
      break;
    // BG0 = Text / 3D, BG1 = Text, BG2 = extended, BG3 = extended
    case 5:
      for (uint i = 0; i < 2; i++) {
        if (mmio.dispcnt.enable[i]) {
          RenderLayerText(i, vcount);
        }
      }
      if (mmio.dispcnt.enable[ENABLE_BG2]) {
        RenderLayerExtended(0);
      }
      if (mmio.dispcnt.enable[ENABLE_BG3]) {
        RenderLayerExtended(1);
      }
      break;
    case 6:
      // TODO: exclude BG1 and BG3 from compositing.
      if (mmio.dispcnt.enable[ENABLE_BG0]) {
        RenderLayerText(0, vcount);
      }
      if (mmio.dispcnt.enable[ENABLE_BG2]) {
        RenderLayerLarge();
      }
      break;
    default:
      ASSERT(false, "PPU: unhandled BG mode {0}", mmio.dispcnt.bg_mode);
  }

  ComposeScanline(vcount, 0, 3);
}

void PPU::RenderVideoMemoryDisplay(u16 vcount) {
  u16 const* source;
  u32* line = &framebuffer[vcount * 256];

  switch (mmio.dispcnt.vram_block) {
    case 0:
      source = reinterpret_cast<u16 const*>(vram.bank_a.data());
      break;
    case 1:
      source = reinterpret_cast<u16 const*>(vram.bank_b.data());
      break;
    case 2:
      source = reinterpret_cast<u16 const*>(vram.bank_c.data());
      break;
    case 3:
      source = reinterpret_cast<u16 const*>(vram.bank_d.data());
      break;
  }

  source += 256 * vcount;

  for (uint x = 0; x < 256; x++) {
    line[x] = ConvertColor(*source++);
  }
}

void PPU::RenderMainMemoryDisplay(u16 vcount) {
  ASSERT(false, "PPU: unimplemented main memory display mode.");
}

} // namespace fauxDS::core
