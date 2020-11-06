/*
 * Copyright (C) 2020 fleroviux
 */

#include <string.h>

#include "ppu.hpp"

namespace fauxDS::core {

PPU::PPU(Region<32> const& vram_bg, Region<16> const& vram_obj, u8 const* pram) 
    : vram_bg(vram_bg)
    , vram_obj(vram_obj)
    , pram(pram) {
  Reset();
}

void PPU::Reset() {
  memset(framebuffer, 0, sizeof(framebuffer));
}

void PPU::RenderScanline(u16 vcount) {
  u32* line = &framebuffer[vcount * 256];

  for (int x = 0; x < 256; x++) {
    line[x] = 0xFFFF0000 | vcount;
  }
}

} // namespace fauxDS::core
