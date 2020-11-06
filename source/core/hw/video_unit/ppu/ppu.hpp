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

private:
  /// Background tile and map data
  Region<32> const& vram_bg;

  /// OBJ tile data
  Region<16> const& vram_obj;

  /// Palette RAM
  u8 const* pram;
};

} // namespace fauxDS::core
