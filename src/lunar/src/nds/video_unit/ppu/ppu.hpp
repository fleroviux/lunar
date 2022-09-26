/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <lunar/integer.hpp>
#include <mutex>
#include <thread>

#include "common/ogl/buffer_object.hpp"
#include "common/ogl/frame_buffer_object.hpp"
#include "common/ogl/program_object.hpp"
#include "common/ogl/texture_2d.hpp"
#include "common/ogl/vertex_array_object.hpp"
#include "common/punning.hpp"
#include "nds/video_unit/gpu/color.hpp"
#include "nds/video_unit/vram.hpp"
#include "registers.hpp"

namespace lunar::nds {

/* 2D picture processing unit (PPU).
 * The Nintendo DS has two PPUs (PPU A and PPU B), one for each screen.
 */
struct PPU {
  PPU(
    int id,
    VRAM const& vram,
    u8   const* pram,
    u8   const* oam,
    Color4 const* gpu_output = nullptr
  );

 ~PPU();

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

    MasterBrightness master_bright;

    bool capture_bg_and_3d;
  } mmio;

  void Reset();

  auto GetOutput() -> u32 const* {
    return &output[frame][0];
  }

  void SwapBuffers() {
    frame ^= 1;
  }

  auto GetComposerOutput() -> u16 const* {
    return &buffer_compose[0];
  }

  void WaitForRenderWorker() {
    while (render_worker.vcount <= render_worker.vcount_max) {}
  }

  void OnWriteVRAM_BG(size_t address_lo, size_t address_hi) {
    OnRegionWrite(vram_bg, render_vram_bg, vram_bg_dirty, {address_lo, address_hi});
  }

  void OnWriteVRAM_OBJ(size_t address_lo, size_t address_hi) {
    OnRegionWrite(vram_obj, render_vram_obj, vram_obj_dirty, {address_lo, address_hi});
  }

  void OnWriteExtPal_BG(size_t address_lo, size_t address_hi) {
    OnRegionWrite(extpal_bg, render_extpal_bg, extpal_bg_dirty, {address_lo, address_hi});
  }

  void OnWriteExtPal_OBJ(size_t address_lo, size_t address_hi) {
    OnRegionWrite(extpal_obj, render_extpal_obj, extpal_obj_dirty, {address_lo, address_hi});
  }

  void OnWriteVRAM_LCDC(size_t address_lo, size_t address_hi) {
    OnRegionWrite(vram_lcdc, render_vram_lcdc, vram_lcdc_dirty, {address_lo, address_hi});
  }

  void OnWritePRAM(size_t address_lo, size_t address_hi) {
    OnRegionWrite(pram, render_pram, pram_dirty, {address_lo, address_hi});
  }

  void OnWriteOAM(size_t address_lo, size_t address_hi) {
    OnRegionWrite(oam, render_oam, oam_dirty, {address_lo, address_hi});
  }

  void OnDrawScanlineBegin(u16 vcount, bool capture_bg_and_3d);
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

  struct AddressRange {
    size_t lo = std::numeric_limits<size_t>::max();
    size_t hi = 0;

    void Expand(AddressRange const& other) {
      lo = std::min(lo, other.lo);
      hi = std::max(hi, other.hi);
    }
  };

  void AffineRenderLoop(
    u16 vcount,
    uint id,
    int  width,
    int  height,
    std::function<void(int, int, int)> render_func
  );

  void RenderScanline(u16 vcount, bool capture_bg_and_3d);
  void RenderDisplayOff(u16 vcount);
  void RenderNormal(u16 vcount);
  void RenderVideoMemoryDisplay(u16 vcount);
  void RenderMainMemoryDisplay(u16 vcount);
  void RenderBackgroundsAndComposite(u16 vcount);
  void RenderMasterBrightness(int vcount);

  void RenderLayerText(uint id, u16 vcount);
  void RenderLayerAffine(uint id, u16 vcount);
  void RenderLayerExtended(uint id, u16 vcount);
  void RenderLayerLarge(u16 vcount);
  void RenderLayerOAM(u16 vcount);
  void RenderWindow(uint id, u8 vcount);

  template<bool window, bool blending, bool opengl>
  void ComposeScanlineTmpl(u16 vcount, int bg_min, int bg_max);
  void ComposeScanline(u16 vcount, int bg_min, int bg_max);
  void Blend(u16 vcount, u16& target1, u16 target2, BlendControl::Effect sfx);

  void SetupRenderWorker();
  void StopRenderWorker();
  void SubmitScanline(u16 vcount, bool capture_bg_and_3d);
  void RegisterMapUnmapCallbacks();

  static auto ConvertColor(u16 color) -> u32 {
    u32 r = (color >>  0) & 0x1F;
    u32 g = (color >>  5) & 0x1F;
    u32 b = (color >> 10) & 0x1F;

    return r << 19 | g << 11 | b << 3 | 0xFF000000;
  }

  auto ReadPalette(uint palette, uint index) -> u16 {
    return *reinterpret_cast<u16 const*>(&render_pram[(palette * 16 + index) * 2]) & 0x7FFF;
  }

  void DecodeTileLine4BPP(u16* buffer, u32 base, uint palette, uint number, uint y, bool flip) {
    uint xor_x = flip ? 7 : 0;
    u32  data  = read<u32>(render_vram_bg, base + number * 32 + y * 4);

    for (uint x = 0; x < 8; x++) {
      auto index = data & 15;
      buffer[x ^ xor_x] = index == 0 ? s_color_transparent : ReadPalette(palette, index);
      data >>= 4;
    }
  }

  void DecodeTileLine8BPP(u16* buffer, u32 base, uint palette, uint extpal_slot, uint number, uint y, bool flip) {
    uint xor_x = flip ? 7 : 0;
    u64  data  = read<u64>(render_vram_bg, base + number * 64 + y * 8);

    for (uint x = 0; x < 8; x++) {
      auto index = data & 0xFF;
      if (index == 0) {
        buffer[x ^ xor_x] = s_color_transparent;
      } else if (mmio.dispcnt.enable_extpal_bg) {
        buffer[x ^ xor_x] = read<u16>(render_extpal_bg, 0x2000 * extpal_slot + (palette * 256 + index) * sizeof(u16));
      } else {
        buffer[x ^ xor_x] = ReadPalette(0, index);
      }
      data >>= 8;
    }
  }

  auto DecodeTilePixel4BPP_OBJ(u32 address, uint palette, int x, int y) -> u16 {
    u8 tuple = read<u8>(render_vram_obj, address + (y * 4) + (x / 2));
    u8 index = (x & 1) ? (tuple >> 4) : (tuple & 0xF);

    if (index == 0) {
      return s_color_transparent;
    } else {
      return ReadPalette(palette, index);
    }
  }

  auto DecodeTilePixel8BPP_BG(u32 address, bool enable_extpal, uint palette, uint extpal_slot, int x, int y) -> u16 {
    u8 index = read<u8>(render_vram_bg, address + (y * 8) + x);

    if (index == 0) {
      return s_color_transparent;
    } else if (enable_extpal && mmio.dispcnt.enable_extpal_bg) {
      return read<u16>(render_extpal_bg, 0x2000 * extpal_slot + (palette * 256 + index) * sizeof(u16));
    } else {
      return ReadPalette(0, index);
    }
  }

  auto DecodeTilePixel8BPP_OBJ(u32 address, uint palette, int x, int y) -> u16 {
    u8 index = read<u8>(render_vram_obj, address + (y * 8) + x);

    if (index == 0) {
      return s_color_transparent;
    } else {
      if (mmio.dispcnt.enable_extpal_obj) {
        return read<u16>(render_extpal_obj, ((palette * 256 + index) * sizeof(u16)) & 0x1FFF);
      } else {
        return ReadPalette(16, index);
      }  
    }
  }

  template<typename T>
  static void CopyVRAM(T const& src, u8* dst, AddressRange const& range) {
    for (size_t address = range.lo; address < range.hi; address++) {
      dst[address] = src.template Read<u8>(address);
    }
  }

  static void CopyVRAM(u8 const* src, u8* dst, AddressRange const& range) {
    for (size_t address = range.lo; address < range.hi; address++) {
      dst[address] = src[address];
    }
  }

  template<typename T>
  void OnRegionWrite(T const& region, u8* copy_dst, AddressRange& dirty_range, AddressRange const& write_range) {
    if (current_vcount < 192) {
      WaitForRenderWorker();
      CopyVRAM(region, copy_dst, write_range);
    } else {
      dirty_range.Expand(write_range);
    }
  }

  void Merge2DWithOpenGL3D();

  int id;
  u32 output[2][256 * 192];
  u16 buffer_compose[256];
  u16 buffer_bg[4][256];
  bool buffer_win[2][256];
  bool window_scanline_enable[2];
  u8 attribute_buffer[256 * 192]; // for OpenGL 3D-to-2D compositing

  struct ObjectPixel {
    u16 color;
    u8  priority;
    unsigned alpha  : 1;
    unsigned window : 1;
  } buffer_obj[256];

  bool line_contains_alpha_obj = false;

  struct RenderWorker {
    std::atomic_int vcount;
    std::atomic_int vcount_max;
    std::atomic_bool running = false;
    std::condition_variable cv;
    std::mutex mutex;
    bool ready;
    std::thread thread;
  } render_worker;

  MMIO mmio_copy[263];

  Region<32> const& vram_bg;  //< Background tile, map and bitmap data
  Region<16> const& vram_obj; //< OBJ tile and bitmap data
  Region<4, 8192> const& extpal_bg;  //< Background extended palette data
  Region<1, 8192> const& extpal_obj; //< OBJ extended palette data
  Region<64> const& vram_lcdc; //< LCDC mapped VRAM
  u8 const* pram; //< Palette RAM
  u8 const* oam;  //< Object Attribute Map

  // Copies of VRAM, PRAM and OAM read by the rendering thread:
  u8 render_vram_bg[524288];
  u8 render_vram_obj[262144];
  u8 render_extpal_bg[32768];
  u8 render_extpal_obj[8192];
  u8 render_vram_lcdc[1048576];
  u8 render_pram[0x400];
  u8 render_oam[0x400];

  // Lowest and highest dirty VRAM addresses
  AddressRange vram_bg_dirty;
  AddressRange vram_obj_dirty;
  AddressRange extpal_bg_dirty;
  AddressRange extpal_obj_dirty;
  AddressRange vram_lcdc_dirty;
  AddressRange pram_dirty;
  AddressRange oam_dirty;

  int current_vcount;

  // Full-frame output of the 3D engine 
  Color4 const* gpu_output;

  int frame = 0;

  // For compositing with OpenGL rendered 3D
  struct OpenGL {
    bool enabled = true; // @todo: set based on GPU renderer configuration
    bool initialized = false;
    bool done = false;
    FrameBufferObject* fbo = nullptr;
    Texture2D* output_texture = nullptr;
    ProgramObject* program = nullptr;
    VertexArrayObject* vao = nullptr;
    BufferObject* vbo = nullptr;
    Texture2D* input_color_texture = nullptr;
    Texture2D* input_attribute_texture = nullptr;

   ~OpenGL() {
      delete fbo;
      delete output_texture;
      delete program;
      delete vao;
      delete vbo;
      delete input_color_texture;
      delete input_attribute_texture;
    }
  } ogl;

  static constexpr u16 s_color_transparent = 0x8000;
  static const int s_obj_size[4][4][2];
};

} // namespace lunar::nds
