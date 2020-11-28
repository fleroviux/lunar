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
  PPU(int id, Region<32> const& vram_bg, Region<16> const& vram_obj, u8 const* pram, u8 const* oam);

  struct MMIO {
    DisplayControl dispcnt;

    BackgroundControl bgcnt[4] { 0, 1, 2, 3 };
    BackgroundOffset bghofs[4];
    BackgroundOffset bgvofs[4];
    RotateScaleParameter bgpa[2];
    RotateScaleParameter bgpb[2];
    RotateScaleParameter bgpc[2];
    RotateScaleParameter bgpd[2];
    ReferencePoint bgx[2];
    ReferencePoint bgy[2];

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
  
  void OnDrawScanlineBegin(u16 vcount);
  void OnDrawScanlineEnd();
  void OnBlankScanlineBegin(u16 vcount);

private:
  // Ugh....
  enum ObjAttribute {
    OBJ_IS_ALPHA  = 1,
    OBJ_IS_WINDOW = 2
  };

  enum ObjectMode {
    OBJ_NORMAL = 0,
    OBJ_SEMI   = 1,
    OBJ_WINDOW = 2,
    OBJ_BITMAP = 3
  };

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

  // FIXME: handle differences between BG and OAM cleanly.

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

  auto DecodeTilePixel4BPP(u32 address, int palette, int x, int y) -> u16 {
    u8 tuple = vram_obj.Read<u8>(address + (y * 4) + (x / 2));
    u8 index = (x & 1) ? (tuple >> 4) : (tuple & 0xF);

    if (index == 0) {
      return s_color_transparent;
    } else {
      return ReadPalette(palette, index);
    }
  }

  auto DecodeTilePixel8BPP(u32 address, int x, int y, bool sprite = false) -> u16 {
    u8 index = vram_bg.Read<u8>(address + (y * 8) + x);

    if (index == 0) {
      return s_color_transparent;
    } else {
      return ReadPalette(sprite ? 16 : 0, index);
    }
  }

  void AffineRenderLoop(uint id,
                        int width,
                        int height,
                        std::function<void(int, int, int)> render_func) {
    auto const& bg = mmio.bgcnt[2 + id];
    auto const& mosaic = mmio.mosaic.bg;
    u16* buffer = buffer_bg[2 + id];
    
    s32 ref_x = mmio.bgx[id]._current;
    s32 ref_y = mmio.bgy[id]._current;
    s16 pa = mmio.bgpa[id].value;
    s16 pc = mmio.bgpc[id].value;
    
    int mosaic_x = 0;
    
    for (int _x = 0; _x < 256; _x++) {
      s32 x = ref_x >> 8;
      s32 y = ref_y >> 8;
      
      if (bg.enable_mosaic) {
        if (++mosaic_x == mosaic.size_x) {
          ref_x += mosaic.size_x * pa;
          ref_y += mosaic.size_x * pc;
          mosaic_x = 0;
        }
      } else {
        ref_x += pa;
        ref_y += pc;
      }
      
      if (bg.wraparound) {
        if (x >= width) {
          x %= width;
        } else if (x < 0) {
          x = width + (x % width);
        }
        
        if (y >= height) {
          y %= height;
        } else if (y < 0) {
          y = height + (y % height);
        }
      } else if (x >= width || y >= height || x < 0 || y < 0) {
        buffer[_x] = s_color_transparent;
        continue;
      }
      
      render_func(_x, (int)x, (int)y);
    }
  }

  void RenderScanline(u16 vcount);

  void RenderDisplayOff(u16 vcount);
  void RenderNormal(u16 vcount);
  void RenderVideoMemoryDisplay(u16 vcount);
  void RenderMainMemoryDisplay(u16 vcount);

  void RenderLayerText(uint id, u16 vcount);
  void RenderLayerAffine(uint id, u16 vcount);
  void RenderLayerOAM(u16 vcount);
  void RenderWindow(uint id, u8 vcount);

  template<bool window, bool blending>
  void ComposeScanlineTmpl(u16 vcount, int bg_min, int bg_max);

  void ComposeScanline(u16 vcount, int bg_min, int bg_max);
  void Blend(u16& target1, u16 target2, BlendControl::Effect sfx);

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

  bool line_contains_alpha_obj = false;

  /// Background tile and map data
  Region<32> const& vram_bg;

  /// OBJ tile data
  Region<16> const& vram_obj;

  /// Palette RAM
  u8 const* pram;

  /// Object Attribute Map
  u8 const* oam;

  static constexpr u16 s_color_transparent = 0x8000;
  static const int s_obj_size[4][4][2];
};

} // namespace fauxDS::core
