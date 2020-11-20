/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>
#include <string.h>

#include "ppu.hpp"

namespace fauxDS::core {

PPU::PPU(int id, Region<32> const& vram_bg, Region<16> const& vram_obj, u8 const* pram) 
    : id(id)
    , vram_bg(vram_bg)
    , vram_obj(vram_obj)
    , pram(pram) {
  if (id == 0) {
    mmio.dispcnt = {};
  } else {
    mmio.dispcnt = {0xC033FFF7};
  }
  InitBlendTable();
  Reset();
}

auto PPU::ConvertColor(u16 color) -> u32 {
  int r = (color >>  0) & 0x1F;
  int g = (color >>  5) & 0x1F;
  int b = (color >> 10) & 0x1F;

  return r << 19 |
         g << 11 |
         b <<  3 |
         0xFF000000;
}

void PPU::Reset() {
  memset(framebuffer, 0, sizeof(framebuffer));

  mmio.dispcnt.Reset();
  
  for (int i = 0; i < 4; i++) {
    mmio.bgcnt[i].Reset();
    mmio.bghofs[i].Reset();
    mmio.bgvofs[i].Reset();
  }

  for (int i = 0; i < 2; i++) {
    mmio.winh[i].Reset();
    mmio.winv[i].Reset();
  }

  mmio.winin.Reset();
  mmio.winout.Reset();

  mmio.bldcnt.Reset();
  mmio.bldalpha.Reset();
  mmio.bldy.Reset();

  mmio.mosaic.Reset();
}

void PPU::RenderScanline(u16 vcount) {
  switch (mmio.dispcnt.display_mode) {
    case 0:
      RenderDisplayOff(vcount);
      break;
    case 1:
      RenderNormal(vcount);
      break;
    case 2:
      RenderVideoMemoryDisplay(vcount);
      break;
    case 3:
      RenderMainMemoryDisplay(vcount);
      break;
  }
}


void PPU::RenderDisplayOff(u16 vcount) {
  u32* line = &framebuffer[vcount * 256];

  for (uint x = 0; x < 256; x++) {
    line[x] = ConvertColor(0x7FFF);
  }
}

void PPU::RenderNormal(u16 vcount) {
  u32* line = &framebuffer[vcount * 256];

  if (mmio.dispcnt.forced_blank) {
    for (uint x = 0; x < 256; x++) {
      line[x] = ConvertColor(0x7FFF);
    }
    return;
  }

  if (mmio.dispcnt.enable[ENABLE_WIN0]) {
    RenderWindow(0, vcount);
  }

  if (mmio.dispcnt.enable[ENABLE_WIN1]) {
    RenderWindow(1, vcount);
  }

  for (uint i = 0; i < 4; i++) {
    if (mmio.dispcnt.enable[i])
      RenderLayerText(i, vcount);
  }

  /*for (uint x = 0; x < 256; x++) {
    u16 color = ReadPalette(0, 0);

    for (int prio = 3; prio >= 0; prio--) {
      for (int bg = 3; bg >= 0; bg--) {
        if (mmio.dispcnt.enable[bg] && mmio.bgcnt[bg].priority == prio && buffer_bg[bg][x] != 0x8000) {
          color = buffer_bg[bg][x];
        }
      }
    }

    line[x] = ConvertColor(color);
  }*/

  ComposeScanline(vcount, 0, 3);
}

void PPU::RenderVideoMemoryDisplay(u16 vcount) {
  ASSERT(false, "PPU: unimplemented video memory display mode.");
}

void PPU::RenderMainMemoryDisplay(u16 vcount) {
  ASSERT(false, "PPU: unimplemented main memory display mode.");
}

void PPU::RenderLayerText(uint id, u16 vcount) {
  auto const& bgcnt = mmio.bgcnt[id];

  u32 tile_base = mmio.dispcnt.tile_block * 65536 + bgcnt.tile_block * 16384;
   
  int line = mmio.bgvofs[id].value + vcount;

  int draw_x = -(mmio.bghofs[id].value % 8);
  int grid_x =   mmio.bghofs[id].value / 8;
  int grid_y = line / 8;
  int tile_y = line % 8;
  
  int screen_x = (grid_x / 32) % 2;
  int screen_y = (grid_y / 32) % 2;

  u16 tile[8];
  u32 base = mmio.dispcnt.map_block * 65536 + bgcnt.map_block * 2048 + (grid_y % 32) * 64;

  u16* buffer = buffer_bg[id];
  s32  last_encoder = -1;
  u16  encoder;
  
  grid_x %= 32;
  
  u32 base_adjust;
  
  switch (bgcnt.size) {
    case 0:
      base_adjust = 0;
      break;
    case 1: 
      base += screen_x * 2048;
      base_adjust = 2048;
      break;
    case 2:
      base += screen_y * 2048;
      base_adjust = 0;
      break;
    case 3:
      base += (screen_x * 2048) + (screen_y * 4096);
      base_adjust = 2048;
      break;
  }
  
  if (screen_x == 1) {
    base_adjust *= -1;
  }
  
  do {
    do {      
      encoder = vram_bg.Read<u16>(base + grid_x++ * 2);

      // TODO: speed tile decoding itself up.
      if (encoder != last_encoder) {
        int number  = encoder & 0x3FF;
        int palette = encoder >> 12;
        bool flip_x = encoder & (1 << 10);
        bool flip_y = encoder & (1 << 11);
        int _tile_y = flip_y ? (tile_y ^ 7) : tile_y;

        if (!bgcnt.full_palette) {
          DecodeTileLine4BPP(tile, tile_base, palette, number, _tile_y, flip_x);
        } else {
          DecodeTileLine8BPP(tile, tile_base, number, _tile_y, flip_x);
        }

        last_encoder = encoder;
      }

      if (draw_x >= 0 && draw_x <= 248) {
        for (int x = 0; x < 8; x++) {
          buffer[draw_x++] = tile[x];
        }
      } else {
        int x = 0;
        int max = 8;

        if (draw_x < 0) {
          x = -draw_x;
          draw_x = 0;
          for (; x < max; x++) {
            buffer[draw_x++] = tile[x];
          }
        } else if (draw_x > 248) {
          max -= draw_x - 248;
          for (; x < max; x++) {
            buffer[draw_x++] = tile[x];
          }
          break;
        }
      }
    } while (grid_x < 32);
    
    base += base_adjust;
    base_adjust *= -1;
    grid_x = 0;
  } while (draw_x < 256);
}

void PPU::RenderWindow(uint id, u8 vcount) {
  auto& winv = mmio.winv[id];

  // Check if the current scanline is outside of the window.
  if ((winv.min <= winv.max && (vcount < winv.min || vcount >= winv.max)) ||
      (winv.min >  winv.max && (vcount < winv.min && vcount >= winv.max))) {
    // Mark window as inactive during the current scanline.
    window_scanline_enable[id] = false;
  } else {
    auto& winh = mmio.winh[id];

    // Mark window as active during the current scanline.
    window_scanline_enable[id] = true;

    // Only recalculate the LUTs if min/max changed between the last update & now.
    if (winh._changed) {
      if (winh.min <= winh.max) {
        for (uint x = 0; x < 256; x++) {
          buffer_win[id][x] = x >= winh.min && x < winh.max;
        }
      } else {
        for (uint x = 0; x < 256; x++) {
          buffer_win[id][x] = x >= winh.min || x < winh.max;
        }
      }
      winh._changed = false;
    }
  }
}

void PPU::ComposeScanline(u16 vcount, int bg_min, int bg_max) {
  u32* line = &framebuffer[vcount * 256];
  u16 backdrop = ReadPalette(0, 0);

  auto const& dispcnt = mmio.dispcnt;
  auto const& bgcnt = mmio.bgcnt;
  auto const& winin = mmio.winin;
  auto const& winout = mmio.winout;

  bool win0_active = dispcnt.enable[ENABLE_WIN0] && window_scanline_enable[0];
  bool win1_active = dispcnt.enable[ENABLE_WIN1] && window_scanline_enable[1];
  bool win2_active = dispcnt.enable[ENABLE_OBJWIN];
  bool no_windows  = !dispcnt.enable[ENABLE_WIN0] &&
                     !dispcnt.enable[ENABLE_WIN1] &&
                     !dispcnt.enable[ENABLE_OBJWIN];

  int bg_list[4];
  int bg_count = 0;

  // Sort enabled backgrounds by their respective priority in ascending order.
  for (int prio = 3; prio >= 0; prio--) {
    for (int bg = bg_max; bg >= bg_min; bg--) {
      if (dispcnt.enable[bg] && bgcnt[bg].priority == prio) {
        bg_list[bg_count++] = bg;
      }
    }
  }

  u16 pixel[2];

  for (uint x = 0; x < 256; x++) {
    int prio[2] = { 4, 4 };
    int layer[2] = { LAYER_BD, LAYER_BD };

    const bool* win_layer_enable = winout.enable[0];

    // Determine the window that has the highest priority.
    if (win0_active && buffer_win[0][x]) {
      win_layer_enable = winin.enable[0];
    } else if (win1_active && buffer_win[1][x]) {
      win_layer_enable = winin.enable[1];
    } else if (win2_active && buffer_obj[x].window) {
      win_layer_enable = winout.enable[1];
    }

    // Find up to two top-most visible background pixels.
    for (int i = 0; i < bg_count; i++) {
      int bg = bg_list[i];

      if (no_windows || win_layer_enable[bg]) {
        auto pixel_new = buffer_bg[bg][x];
        if (pixel_new != s_color_transparent) {
          layer[1] = layer[0];
          layer[0] = bg;
          prio[1] = prio[0];
          prio[0] = bgcnt[bg].priority;
        }
      }
    }

    /* Check if a OBJ pixel takes priority over one of the two
     * top-most background pixels and insert it accordingly.
     */
    /*if (dispcnt.enable[ENABLE_OBJ] &&
        buffer_obj[x].color != s_color_transparent &&
        (no_windows || win_layer_enable[LAYER_OBJ])) {
      int priority = buffer_obj[x].priority;

      if (priority <= prio[0]) {
        layer[1] = layer[0];
        layer[0] = LAYER_OBJ;
      } else if (priority <= prio[1]) {
        layer[1] = LAYER_OBJ;
      }
    }*/

    // Map layer numbers to pixels.
    for (int i = 0; i < 2; i++) {
      int _layer = layer[i];
      switch (_layer) {
        case 0:
        case 1:
        case 2:
        case 3:
          pixel[i] = buffer_bg[_layer][x];
          break;
        case 4:
          pixel[i] = buffer_obj[x].color;
          break;
        case 5:
          pixel[i] = backdrop;
          break;
      }
    }

    bool is_alpha_obj = layer[0] == LAYER_OBJ && buffer_obj[x].alpha;

    if (no_windows || win_layer_enable[LAYER_SFX] || is_alpha_obj) {
      auto blend_mode = mmio.bldcnt.sfx;
      bool have_dst = mmio.bldcnt.targets[0][layer[0]];
      bool have_src = mmio.bldcnt.targets[1][layer[1]];

      if (is_alpha_obj && have_src) {
        Blend(pixel[0], pixel[1], BlendControl::Effect::SFX_BLEND);
      } else if (have_dst && blend_mode != BlendControl::Effect::SFX_NONE && (have_src || blend_mode != BlendControl::Effect::SFX_BLEND)) {
        Blend(pixel[0], pixel[1], blend_mode);
      }
    }

    line[x] = ConvertColor(pixel[0]);
  }
}

void PPU::InitBlendTable() {
  for (int color0 = 0; color0 <= 31; color0++) {
    for (int color1 = 0; color1 <= 31; color1++) {
      for (int factor0 = 0; factor0 <= 16; factor0++) {
        for (int factor1 = 0; factor1 <= 16; factor1++) {
          int result = (color0 * factor0 + color1 * factor1) >> 4;

          blend_table[factor0][factor1][color0][color1] = std::min<u8>(result, 31);
        }
      }
    }
  }
}

void PPU::Blend(u16& target1, u16 target2, BlendControl::Effect sfx) {
  int r1 = (target1 >>  0) & 0x1F;
  int g1 = (target1 >>  5) & 0x1F;
  int b1 = (target1 >> 10) & 0x1F;

  switch (sfx) {
    case BlendControl::Effect::SFX_BLEND: {
      int eva = std::min<int>(16, mmio.bldalpha.a);
      int evb = std::min<int>(16, mmio.bldalpha.b);

      int r2 = (target2 >>  0) & 0x1F;
      int g2 = (target2 >>  5) & 0x1F;
      int b2 = (target2 >> 10) & 0x1F;

      r1 = blend_table[eva][evb][r1][r2];
      g1 = blend_table[eva][evb][g1][g2];
      b1 = blend_table[eva][evb][b1][b2];
      break;
    }
    case BlendControl::Effect::SFX_BRIGHTEN: {
      int evy = std::min<int>(16, mmio.bldy.y);

      r1 = blend_table[16 - evy][evy][r1][31];
      g1 = blend_table[16 - evy][evy][g1][31];
      b1 = blend_table[16 - evy][evy][b1][31];
      break;
    }
    case BlendControl::Effect::SFX_DARKEN: {
      int evy = std::min<int>(16, mmio.bldy.y);

      r1 = blend_table[16 - evy][evy][r1][0];
      g1 = blend_table[16 - evy][evy][g1][0];
      b1 = blend_table[16 - evy][evy][b1][0];
      break;
    }
  }

  target1 = r1 | (g1 << 5) | (b1 << 10);
}

} // namespace fauxDS::core
