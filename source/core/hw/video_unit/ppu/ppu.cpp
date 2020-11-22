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

void PPU::OnDrawScanlineBegin(u16 vcount) {
  if (mmio.dispcnt.enable[ENABLE_WIN0]) {
    RenderWindow(0, vcount);
  }

  if (mmio.dispcnt.enable[ENABLE_WIN1]) {
    RenderWindow(1, vcount);
  }

  RenderScanline(vcount);
}

void PPU::OnBlankScanlineBegin(u16 vcount) {
  if (mmio.dispcnt.enable[ENABLE_WIN0]) {
    RenderWindow(0, vcount);
  }

  if (mmio.dispcnt.enable[ENABLE_WIN1]) {
    RenderWindow(1, vcount);
  }
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

  for (uint i = 0; i < 4; i++) {
    if (mmio.dispcnt.enable[i])
      RenderLayerText(i, vcount);
  }

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
  auto& winh = mmio.winh[id];

  if (vcount == winv.min) {
    window_scanline_enable[id] = true;
  }

  if (vcount == winv.max) {
    window_scanline_enable[id] = false;
  }

  if (window_scanline_enable[id] && winh._changed) {
    // TODO: X1=00h is treated as 0 (left-most), X2=00h is treated as 100h (right-most).
    // However, the window is not displayed if X1=X2=00h
    if (winh.min <= winh.max) {
      for (int x = 0; x < 240; x++) {
        buffer_win[id][x] = x >= winh.min && x < winh.max;
      }
    } else {
      for (int x = 0; x < 240; x++) {
        buffer_win[id][x] = x >= winh.min || x < winh.max;
      }
    }
    
    winh._changed = false;
  }
}

template<bool window, bool blending>
void PPU::ComposeScanlineTmpl(u16 vcount, int bg_min, int bg_max) {
  u32* line = &framebuffer[vcount * 256];
  u16 backdrop = ReadPalette(0, 0);

  auto const& dispcnt = mmio.dispcnt;
  auto const& bgcnt = mmio.bgcnt;
  auto const& winin = mmio.winin;
  auto const& winout = mmio.winout;

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

  bool win0_active = false;
  bool win1_active = false;
  bool win2_active = false;

  const bool* win_layer_enable;
  
  if constexpr (window) {
    win0_active = dispcnt.enable[ENABLE_WIN0] && window_scanline_enable[0];
    win1_active = dispcnt.enable[ENABLE_WIN1] && window_scanline_enable[1];
    win2_active = dispcnt.enable[ENABLE_OBJWIN];
  }

  int prio[2];
  int layer[2];
  u16 pixel[2];

  for (int x = 0; x < 256; x++) {
    if constexpr (window) {
      // Determine the window with the highest priority for this pixel.
      if (win0_active && buffer_win[0][x]) {
        win_layer_enable = winin.enable[0];
      } else if (win1_active && buffer_win[1][x]) {
        win_layer_enable = winin.enable[1];
      } else if (win2_active && buffer_obj[x].window) {
        win_layer_enable = winout.enable[1];
      } else {
        win_layer_enable = winout.enable[0];
      }
    }

    if constexpr (blending) {
      bool is_alpha_obj = false;

      prio[0] = 4;
      prio[1] = 4;
      layer[0] = LAYER_BD;
      layer[1] = LAYER_BD;
      
      // Find up to two top-most visible background pixels.
      for (int i = 0; i < bg_count; i++) {
        int bg = bg_list[i];

        if (!window || win_layer_enable[bg]) {
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
      /*if ((!window || win_layer_enable[LAYER_OBJ]) &&
          dispcnt.enable[ENABLE_OBJ] &&
          buffer_obj[x].color != s_color_transparent) {
        int priority = buffer_obj[x].priority;

        if (priority <= prio[0]) {
          layer[1] = layer[0];
          layer[0] = LAYER_OBJ;
          is_alpha_obj = buffer_obj[x].alpha;
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

      if (!window || win_layer_enable[LAYER_SFX] || is_alpha_obj) {
        auto blend_mode = mmio.bldcnt.sfx;
        bool have_dst = mmio.bldcnt.targets[0][layer[0]];
        bool have_src = mmio.bldcnt.targets[1][layer[1]];

        if (is_alpha_obj && have_src) {
          Blend(pixel[0], pixel[1], BlendControl::Effect::SFX_BLEND);
        } else if (have_dst && blend_mode != BlendControl::Effect::SFX_NONE && (have_src || blend_mode != BlendControl::Effect::SFX_BLEND)) {
          Blend(pixel[0], pixel[1], blend_mode);
        }
      }
    } else {
      pixel[0] = backdrop;
      prio[0] = 4;
      
      // Find the top-most visible background pixel.
      if (bg_count != 0) {
        for (int i = bg_count - 1; i >= 0; i--) {
          int bg = bg_list[i];

          if (!window || win_layer_enable[bg]) {
            u16 pixel_new = buffer_bg[bg][x];
            if (pixel_new != s_color_transparent) {
              pixel[0] = pixel_new;
              prio[0] = bgcnt[bg].priority;
              break;
            }   
          }
        }
      }

      // Check if a OBJ pixel takes priority over the top-most background pixel.
     /* if ((!window || win_layer_enable[LAYER_OBJ]) &&
          dispcnt.enable[ENABLE_OBJ] &&
          buffer_obj[x].color != s_color_transparent &&
          buffer_obj[x].priority <= prio[0]) {
        pixel[0] = buffer_obj[x].color;
      }*/
    }

    line[x] = ConvertColor(pixel[0]);
  }
}

void PPU::ComposeScanline(u16 vcount, int bg_min, int bg_max) {
    auto const& dispcnt = mmio.dispcnt;

  int key = 0;

  if (dispcnt.enable[ENABLE_WIN0] ||
      dispcnt.enable[ENABLE_WIN1] ||
      dispcnt.enable[ENABLE_OBJWIN]) {
    key |= 1;
  }

  if (mmio.bldcnt.sfx != BlendControl::Effect::SFX_NONE || line_contains_alpha_obj) {
    key |= 2;
  }

  switch (key) {
    case 0b00:
      ComposeScanlineTmpl<false, false>(vcount, bg_min, bg_max);
      break;
    case 0b01:
      ComposeScanlineTmpl<true, false>(vcount, bg_min, bg_max);
      break;
    case 0b10:
      ComposeScanlineTmpl<false, true>(vcount, bg_min, bg_max);
      break;
    case 0b11:
      ComposeScanlineTmpl<true, true>(vcount, bg_min, bg_max);
      break;
  }
}

void PPU::Blend(u16& target1,
                u16  target2,
                BlendControl::Effect sfx) {
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

      r1 = std::min<u8>((r1 * eva + r2 * evb) >> 4, 31);
      g1 = std::min<u8>((g1 * eva + g2 * evb) >> 4, 31);
      b1 = std::min<u8>((b1 * eva + b2 * evb) >> 4, 31);
      break;
    }
    case BlendControl::Effect::SFX_BRIGHTEN: {
      int evy = std::min<int>(16, mmio.bldy.y);
      
      r1 += ((31 - r1) * evy) >> 4;
      g1 += ((31 - g1) * evy) >> 4;
      b1 += ((31 - b1) * evy) >> 4;
      break;
    }
    case BlendControl::Effect::SFX_DARKEN: {
      int evy = std::min<int>(16, mmio.bldy.y);
      
      r1 -= (r1 * evy) >> 4;
      g1 -= (g1 * evy) >> 4;
      b1 -= (b1 * evy) >> 4;
      break;
    }
  }

  target1 = r1 | (g1 << 5) | (b1 << 10);
}

} // namespace fauxDS::core
