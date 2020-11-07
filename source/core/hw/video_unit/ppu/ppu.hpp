/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <core/hw/video_unit/vram_region.hpp>

#include "registers.hpp"

namespace fauxDS::core {

/// 2D picture processing unit (PPU).
/// The Nintendo DS has two of these (PPU A and PPU B) for each screen.
struct PPU {
  PPU(Region<32> const& vram_bg, Region<16> const& vram_obj, u8 const* pram);

  struct MMIO {
    BackgroundControl bgcnt[4] { 0, 1, 2, 3 };

    BackgroundOffset bghofs[4] {};
    BackgroundOffset bgvofs[4] {};
  } mmio;

  void Reset();
  auto GetFramebuffer() -> u32* { return &framebuffer[0]; }
  void RenderScanline(u16 vcount);

private:
  auto ReadPalette(uint palette, uint index) -> u16 {
    return *reinterpret_cast<u16 const*>(pram[(palette * 16 + index) * 2]) & 0x7FFF;
  }

  /*void DecodeTileLine4BPP(
    u16* buffer,
    u32 base,
    uint palette,
    uint number,
    uint y,
    bool flip
  ) {
    std::uint8_t* data = &vram[base + (number * 32) + (y * 4)];

    if (flip) {
      for (int x = 0; x < 4; x++) {
        int d  = *data++;
        int p1 = d & 15;
        int p2 = d >> 4;

        buffer[(x*2+0)^7] = p1 ? ReadPalette(palette, p1) : s_color_transparent;
        buffer[(x*2+1)^7] = p2 ? ReadPalette(palette, p2) : s_color_transparent;
      }
    } else {
      for (int x = 0; x < 4; x++) {
        int d  = *data++;
        int p1 = d & 15;
        int p2 = d >> 4;

        buffer[x*2+0] = p1 ? ReadPalette(palette, p1) : s_color_transparent;
        buffer[x*2+1] = p2 ? ReadPalette(palette, p2) : s_color_transparent;
      }
    }
  }*/

  u32 framebuffer[256 * 192];

  /// Background tile and map data
  Region<32> const& vram_bg;

  /// OBJ tile data
  Region<16> const& vram_obj;

  /// Palette RAM
  u8 const* pram;
};

} // namespace fauxDS::core
