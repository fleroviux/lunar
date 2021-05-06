/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <util/integer.hpp>
#include <functional>

#include "hw/video_unit/gpu/color.hpp"
#include "hw/video_unit/vram.hpp"
#include "registers.hpp"

namespace Duality::Core {

/// 2D picture processing unit (PPU).
/// The Nintendo DS has two of these (PPU A and PPU B) for each screen.
struct PPU {
  PPU(
    int id,
    VRAM const& vram,
    u8   const* pram,
    u8   const* oam,
    Color4 const* gpu_output = nullptr);

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
  auto GetOutput() -> u32 const* { return &output[0]; }
  
  void OnDrawScanlineBegin(u16 vcount);
  void OnDrawScanlineEnd();
  void OnBlankScanlineBegin(u16 vcount);

private:
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

  void AffineRenderLoop(
    uint id,
    int  width,
    int  height,
    std::function<void(int, int, int)> render_func);

  void RenderScanline(u16 vcount);
  void RenderDisplayOff(u16 vcount);
  void RenderNormal(u16 vcount);
  void RenderVideoMemoryDisplay(u16 vcount);
  void RenderMainMemoryDisplay(u16 vcount);

  void RenderLayerText(uint id, u16 vcount);
  void RenderLayerAffine(uint id);
  void RenderLayerExtended(uint id);
  void RenderLayerLarge();
  void RenderLayerOAM(u16 vcount);
  void RenderWindow(uint id, u8 vcount);

  template<bool window, bool blending>
  void ComposeScanlineTmpl(u16 vcount, int bg_min, int bg_max);
  void ComposeScanline(u16 vcount, int bg_min, int bg_max);
  void Blend(u16& target1, u16 target2, BlendControl::Effect sfx);

  static auto ConvertColor(u16 color) -> u32 {
    u32 r = (color >>  0) & 0x1F;
    u32 g = (color >>  5) & 0x1F;
    u32 b = (color >> 10) & 0x1F;

    return r << 19 | g << 11 | b << 3 | 0xFF000000;
  }

  auto ReadPalette(uint palette, uint index) -> u16 {
    return *reinterpret_cast<u16 const*>(&pram[(palette * 16 + index) * 2]) & 0x7FFF;
  }

  void DecodeTileLine4BPP(u16* buffer, u32 base, uint palette, uint number, uint y, bool flip) {
    uint xor_x = flip ? 7 : 0;
    u32  data  = vram_bg.Read<u32>(base + number * 32 + y * 4);

    for (uint x = 0; x < 8; x++) {
      auto index = data & 15;
      buffer[x ^ xor_x] = index == 0 ? s_color_transparent : ReadPalette(palette, index);
      data >>= 4;
    }
  }

  void DecodeTileLine8BPP(u16* buffer, u32 base, uint palette, uint extpal_slot, uint number, uint y, bool flip) {
    uint xor_x = flip ? 7 : 0;
    u64  data  = vram_bg.Read<u64>(base + number * 64 + y * 8);

    for (uint x = 0; x < 8; x++) {
      auto index = data & 0xFF;
      if (index == 0) {
        buffer[x ^ xor_x] = s_color_transparent;
      } else if (mmio.dispcnt.enable_extpal_bg) {
        buffer[x ^ xor_x] = extpal_bg.Read<u16>(0x2000 * extpal_slot + (palette * 256 + index) * sizeof(u16));
      } else {
        buffer[x ^ xor_x] = ReadPalette(0, index);
      }
      data >>= 8;
    }
  }

  auto DecodeTilePixel4BPP_OBJ(u32 address, uint palette, int x, int y) -> u16 {
    u8 tuple = vram_obj.Read<u8>(address + (y * 4) + (x / 2));
    u8 index = (x & 1) ? (tuple >> 4) : (tuple & 0xF);

    if (index == 0) {
      return s_color_transparent;
    } else {
      return ReadPalette(palette, index);
    }
  }

  auto DecodeTilePixel8BPP_BG(u32 address, bool enable_extpal, uint palette, uint extpal_slot, int x, int y) -> u16 {
    u8 index = vram_bg.Read<u8>(address + (y * 8) + x);

    if (index == 0) {
      return s_color_transparent;
    } else if (enable_extpal && mmio.dispcnt.enable_extpal_bg) {
      return extpal_bg.Read<u16>(0x2000 * extpal_slot + (palette * 256 + index) * sizeof(u16));
    } else {
      return ReadPalette(0, index);
    }
  }

  auto DecodeTilePixel8BPP_OBJ(u32 address, uint palette, int x, int y) -> u16 {
    u8 index = vram_obj.Read<u8>(address + (y * 8) + x);

    if (index == 0) {
      return s_color_transparent;
    } else {
      if (mmio.dispcnt.enable_extpal_obj) {
        return extpal_obj.Read<u16>((palette * 256 + index) * sizeof(u16));
      } else {
        return ReadPalette(16, index);
      }  
    }
  }

  int id;
  u32 output[256 * 192];
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

  /// Background tile, map and bitmap data
  Region<32> const& vram_bg;

  /// OBJ tile and bitmap data
  Region<16> const& vram_obj;

  /// Background extended palette data
  Region<4, 8192> const& extpal_bg;

  /// OBJ extended palette data
  Region<1, 8192> const& extpal_obj;

  /// LCDC mapped VRAM
  Region<64> const& vram_lcdc;

  /// Palette RAM
  u8 const* pram;

  /// Object Attribute Map
  u8 const* oam;

  /// Full-frame output of the 3D engine 
  Color4 const* gpu_output;

  static constexpr u16 s_color_transparent = 0x8000;
  static const int s_obj_size[4][4][2];
};

} // namespace Duality::Core
