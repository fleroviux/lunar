/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>
#include <string.h>

#include "ppu.hpp"

namespace fauxDS::core {

PPU::PPU(int id, Region<32> const& vram_bg, Region<16> const& vram_obj, u8 const* pram) 
    : id(id)
    , vram_bg(vram_bg)
    , vram_obj(vram_obj)
    , pram(pram) {
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

void PPU::OnBlankScanlineBegin(u16 vcount) {
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

  for (uint i = 0; i < 4; i++) {
    if (mmio.dispcnt.enable[i])
      RenderLayerText(i, vcount);
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
