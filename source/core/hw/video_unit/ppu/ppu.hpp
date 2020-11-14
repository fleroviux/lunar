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
  inline auto ReadPalette(uint palette, uint index) -> u16 {
    return *reinterpret_cast<u16 const*>(&pram[(palette * 16 + index) * 2]) & 0x7FFF;
  }

  inline void DecodeTileLine4BPP(u16* buffer, u32 base, uint palette, uint number, uint y, bool flip) {
    uint xor_x = flip ? 7 : 0;
    u32  data  = vram_bg.Read<u32>(base + number * 32 + y * 4);

    for (uint x = 0; x < 8; x++) {
      auto index = data & 15;
      buffer[x ^ xor_x] = index == 0 ? 0x8000 : ReadPalette(palette, index);
      data >>= 4;
    }
  }

  inline void DecodeTileLine8BPP(u16* buffer, u32 base, uint number, uint y, bool flip) {
    uint xor_x = flip ? 7 : 0;
    u64  data  = vram_bg.Read<u64>(base + number * 64 + y * 8);

    for (uint x = 0; x < 8; x++) {
      auto index = data & 0xFF;
      buffer[x ^ xor_x] = index == 0 ? 0x8000 : ReadPalette(0, index);
      data >>= 8;
    }
  }

  void RenderLayerText(uint id, u16 vcount);

  u32 framebuffer[256 * 192];
  u16 buffer_bg[4][256];

  /// Background tile and map data
  Region<32> const& vram_bg;

  /// OBJ tile data
  Region<16> const& vram_obj;

  /// Palette RAM
  u8 const* pram;
};

} // namespace fauxDS::core
