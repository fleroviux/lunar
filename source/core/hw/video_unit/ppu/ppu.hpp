/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <core/hw/video_unit/vram_region.hpp>

namespace fauxDS::core {

struct PPU {
  PPU(Region<32> const& vram_bg, Region<16> const& vram_obj, u8 const* pram);

  void Reset();
  auto GetFramebuffer() -> u32* { return &framebuffer[0]; }
  void RenderScanline(u16 vcount);

private:
  u32 framebuffer[256 * 192];

  /// Background tile and map data
  Region<32> const& vram_bg;

  /// OBJ tile data
  Region<16> const& vram_obj;

  /// Palette RAM
  u8 const* pram;
};

} // namespace fauxDS::core
