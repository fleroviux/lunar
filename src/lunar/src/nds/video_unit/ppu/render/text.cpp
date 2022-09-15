
// Copyright (C) 2022 fleroviux

#include "common/punning.hpp"
#include "nds/video_unit/ppu/ppu.hpp"

namespace lunar::nds {

void PPU::RenderLayerText(uint id, u16 vcount) {
  auto const& mmio = mmio_copy[vcount];
  auto const& bgcnt = mmio.bgcnt[id];
  auto const& mosaic = mmio.mosaic.bg;

  uint expal_slot = id | (bgcnt.palette_slot << 1);

  u32 tile_base = mmio.dispcnt.tile_block * 65536 + bgcnt.tile_block * 16384;
   
  int line = mmio.bgvofs[id].value + vcount;

  // Apply vertical mosaic
  if (bgcnt.enable_mosaic) {
    line -= mosaic._counter_y;
  }

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
      encoder = read<u16>(render_vram_bg, base + grid_x++ * 2);

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
          DecodeTileLine8BPP(tile, tile_base, palette, expal_slot, number, _tile_y, flip_x);
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

  // Apply horizontal mosaic
  if (bgcnt.enable_mosaic && mosaic.size_x != 1) {
    int mosaic_x = 0;

    for (int x = 0; x < 256; x++) {
      buffer[x] = buffer[x - mosaic_x];
      if (++mosaic_x == mosaic.size_x) {
        mosaic_x = 0;
      }
    }
  }
}

} // namespace lunar::nds
