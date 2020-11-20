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
  PPU(int id, Region<32> const& vram_bg, Region<16> const& vram_obj, u8 const* pram);

  struct MMIO {
    DisplayControl dispcnt;

    BackgroundControl bgcnt[4] { 0, 1, 2, 3 };
    BackgroundOffset bghofs[4];
    BackgroundOffset bgvofs[4];

    WindowRange winh[2];
    WindowRange winv[2];
    WindowLayerSelect winin;
    WindowLayerSelect winout;

    BlendControl bldcnt;
    BlendAlpha bldalpha;
    BlendBrightness bldy;

    Mosaic mosaic;
  } mmio;

  void Reset();
  auto GetFramebuffer() -> u32* { return &framebuffer[0]; }
  void RenderScanline(u16 vcount);

private:
  enum Layer {
    LAYER_BG0 = 0,
    LAYER_BG1 = 1,
    LAYER_BG2 = 2,
    LAYER_BG3 = 3,
    LAYER_OBJ = 4,
    LAYER_SFX = 5,
    LAYER_BD  = 5
  };

  enum Enable {
    ENABLE_BG0 = 0,
    ENABLE_BG1 = 1,
    ENABLE_BG2 = 2,
    ENABLE_BG3 = 3,
    ENABLE_OBJ = 4,
    ENABLE_WIN0 = 5,
    ENABLE_WIN1 = 6,
    ENABLE_OBJWIN = 7
  };

  static auto ConvertColor(u16 color) -> u32;

  inline auto ReadPalette(uint palette, uint index) -> u16 {
    return *reinterpret_cast<u16 const*>(&pram[(palette * 16 + index) * 2]) & 0x7FFF;
  }

  inline void DecodeTileLine4BPP(u16* buffer, u32 base, uint palette, uint number, uint y, bool flip) {
    uint xor_x = flip ? 7 : 0;
    u32  data  = vram_bg.Read<u32>(base + number * 32 + y * 4);

    for (uint x = 0; x < 8; x++) {
      auto index = data & 15;
      buffer[x ^ xor_x] = index == 0 ? s_color_transparent : ReadPalette(palette, index);
      data >>= 4;
    }
  }

  inline void DecodeTileLine8BPP(u16* buffer, u32 base, uint number, uint y, bool flip) {
    uint xor_x = flip ? 7 : 0;
    u64  data  = vram_bg.Read<u64>(base + number * 64 + y * 8);

    for (uint x = 0; x < 8; x++) {
      auto index = data & 0xFF;
      buffer[x ^ xor_x] = index == 0 ? s_color_transparent : ReadPalette(0, index);
      data >>= 8;
    }
  }

  void RenderDisplayOff(u16 vcount);
  void RenderNormal(u16 vcount);
  void RenderVideoMemoryDisplay(u16 vcount);
  void RenderMainMemoryDisplay(u16 vcount);

  void RenderLayerText(uint id, u16 vcount);
  void RenderWindow(uint id, u8 value);

  void ComposeScanline(u16 vcount, int bg_min, int bg_max);
  void InitBlendTable();
  void Blend(u16& target1, u16 target2, BlendControl::Effect sfx);

  u8 blend_table[17][17][32][32];

  int id;
  u32 framebuffer[256 * 192];
  u16 buffer_bg[4][256];
  bool buffer_win[2][256];
  bool window_scanline_enable[2];

  struct ObjectPixel {
    u16 color;
    u8  priority;
    unsigned alpha  : 1;
    unsigned window : 1;
  } buffer_obj[256];

  /// Background tile and map data
  Region<32> const& vram_bg;

  /// OBJ tile data
  Region<16> const& vram_obj;

  /// Palette RAM
  u8 const* pram;

  static constexpr u16 s_color_transparent = 0x8000;
};

} // namespace fauxDS::core
