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
  mmio = {};
}

void PPU::RenderScanline(u16 vcount) {
  u32* line = &framebuffer[vcount * 256];

  for (int x = 0; x < 256; x++) {
    auto block_x = x / 8;
    auto block_y = vcount / 8;
    auto tile_x = x % 8;
    auto tile_y = vcount % 8;

    u16 encoder = vram_bg.Read<u16>(22 * 2048 + block_y * 64 + block_x * 2);

    u16 tile = encoder & 0x3FF;//block_y * 32 + block_x;
    u16 const* palette = (const u16*)&pram[(encoder >> 12) * 32];

    u32 offset = 16384 * 3 + tile * 32 + tile_y * 4 + tile_x / 2;
    u8 value = vram_bg.Read<u8>(offset);

    if (x & 1)
      value >>= 4;
    else 
      value &= 15;

    u16 color = palette[value];
    line[x] = (((color >>  0) & 0x1F) << 19) |
              (((color >>  5) & 0x1F) << 11) |
              (((color >> 10) & 0x1F) <<  3) |
              0xFF000000;

    //line[x] = 0xFFFF0000 | vcount;
  }
}

} // namespace fauxDS::core
