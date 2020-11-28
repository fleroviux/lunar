/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>
#include <string.h>

#include "ppu.hpp"

namespace fauxDS::core {

PPU::PPU(int id, Region<32> const& vram_bg, Region<16> const& vram_obj, u8 const* pram, u8 const* oam) 
    : id(id)
    , vram_bg(vram_bg)
    , vram_obj(vram_obj)
    , pram(pram)
    , oam(oam) {
  if (id == 0) {
    mmio.dispcnt = {};
  } else {
    mmio.dispcnt = {0xC033FFF7};
  }
  Reset();
}

auto PPU::ConvertColor(u16 color) -> u32 {
  int r = (color >>  0) & 0x1F;
  int g = (color >>  5) & 0x1F;
  int b = (color >> 10) & 0x1F;

  return r << 19 |
         g << 11 |
         b <<  3 |
         0xFF000000;
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
  auto& dispcnt = mmio.dispcnt;
  auto& bgx = mmio.bgx;
  auto& bgy = mmio.bgy;
  auto& mosaic = mmio.mosaic;

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

  if (dispcnt.enable[ENABLE_WIN0]) {
    RenderWindow(0, vcount);
  }

  if (dispcnt.enable[ENABLE_WIN1]) {
    RenderWindow(1, vcount);
  }

  RenderScanline(vcount);
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

  if (mmio.dispcnt.enable[ENABLE_OBJ]) {
    RenderLayerOAM(vcount);
  }

  switch (mmio.dispcnt.bg_mode) {
    // BG0 = Text/3D, BG1 - BG3 = Text
    case 0:
      for (uint i = 0; i < 4; i++) {
        if (mmio.dispcnt.enable[i])
          RenderLayerText(i, vcount);
      }
      break;
    // BG0 = Text/3D, BG1 - BG2 = Text, BG = affine
    case 1:
      for (uint i = 0; i < 3; i++) {
        if (mmio.dispcnt.enable[i])
          RenderLayerText(i, vcount);
      }
      if (mmio.dispcnt.enable[ENABLE_BG3])
        RenderLayerAffine(0, vcount);
      break;
    // BG0 = Text/3D, BG1 = Text, BG2 - BG3 = affine
    case 2:
      for (uint i = 0; i < 2; i++) {
        if (mmio.dispcnt.enable[i])
          RenderLayerText(i, vcount);
      }
      for (uint i = 0; i < 2; i++) {
        if (mmio.dispcnt.enable[2 + i])
          RenderLayerAffine(i, vcount);
      }
      break;
    default:
      ASSERT(false, "PPU: unhandled BG mode {0}", mmio.dispcnt.bg_mode);
  }

  ComposeScanline(vcount, 0, 3);
}

void PPU::RenderVideoMemoryDisplay(u16 vcount) {
  ASSERT(false, "PPU: unimplemented video memory display mode.");
}

void PPU::RenderMainMemoryDisplay(u16 vcount) {
  ASSERT(false, "PPU: unimplemented main memory display mode.");
}

} // namespace fauxDS::core
