
// Copyright (C) 2022 fleroviux

#include "common/punning.hpp"
#include "nds/video_unit/ppu/ppu.hpp"

namespace lunar::nds {

void PPU::AffineRenderLoop(
  u16 vcount,
  uint id,
  int  width,
  int  height,
  std::function<void(int, int, int)> render_func
) {
  auto const& mmio = mmio_copy[vcount];
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

void PPU::RenderLayerAffine(uint id, u16 vcount) {
  auto const& mmio = mmio_copy[vcount];
  auto const& bg = mmio.bgcnt[2 + id];
  
  u16* buffer = buffer_bg[2 + id];
  
  int size = 128 << bg.size;
  int block_width = 16 << bg.size;
  u32 map_base  = mmio.dispcnt.map_block  * 65536 + bg.map_block  * 2048;
  u32 tile_base = mmio.dispcnt.tile_block * 65536 + bg.tile_block * 16384;
  
  AffineRenderLoop(vcount, id, size, size, [&](int line_x, int x, int y) {
    auto tile_number = read<u8>(render_vram_bg, map_base + (y >> 3) * block_width + (x >> 3));
    buffer[line_x] = DecodeTilePixel8BPP_BG(
      tile_base + tile_number * 64,
      false,
      0,
      0,
      x & 7,
      y & 7
    );
  });
}

void PPU::RenderLayerExtended(uint id, u16 vcount) {
  auto const& mmio = mmio_copy[vcount];
  auto const& bg = mmio.bgcnt[2 + id];

  u16* buffer = buffer_bg[2 + id];

  if (bg.full_palette) {
    int width;
    int height;

    switch (bg.size) {
      case 0: width = 128; height = 128; break;
      case 1: width = 256; height = 256; break;
      case 2: width = 512; height = 256; break;
      case 3: width = 512; height = 512; break;
    }

    if (bg.tile_block & 1) {
      // Rotate/Scale direct color bitmap
      AffineRenderLoop(vcount, id, width, height, [&](int line_x, int x, int y) {
        u16 color = read<u16>(render_vram_bg, bg.map_block * 16384 + (y * width + x) * 2);
        if (color & 0x8000) {
          buffer[line_x] = color & 0x7FFF;
        } else {
          buffer[line_x] = s_color_transparent;
        }
      });
    } else {
      // Rotate/Scale 256-color bitmap
      AffineRenderLoop(vcount, id, width, height, [&](int line_x, int x, int y) {
        u8 index = read<u8>(render_vram_bg, bg.map_block * 16384 + y * width + x);
        if (index == 0) {
          buffer[line_x] = s_color_transparent;
        } else {
          buffer[line_x] = ReadPalette(0, index);
        }
      });
    }

  } else {
    // Rotate/Scale with 16-bit background map entries (Text + Affine mixup)
    int size = 128 << bg.size;
    int block_width = 16 << bg.size;
    u32 map_base  = mmio.dispcnt.map_block  * 65536 + bg.map_block  * 2048;
    u32 tile_base = mmio.dispcnt.tile_block * 65536 + bg.tile_block * 16384;
      
    AffineRenderLoop(vcount, id, size, size, [&](int line_x, int x, int y) {
      u16 encoder = read<u16>(render_vram_bg, map_base + ((y >> 3) * block_width + (x >> 3)) * 2);
      int number  = encoder & 0x3FF;
      int palette = encoder >> 12;
      int tile_x = x & 7;
      int tile_y = y & 7;

      if (encoder & (1 << 10)) tile_x = 7 - tile_x;
      if (encoder & (1 << 11)) tile_y = 7 - tile_y;
 
      buffer[line_x] = DecodeTilePixel8BPP_BG(tile_base + number * 64, true, palette, 2 + id, tile_x, tile_y);
    });
  }
}

void PPU::RenderLayerLarge(u16 vcount) {
  auto const& mmio = mmio_copy[vcount];
  auto const& bg = mmio.bgcnt[2];

  int width = 512 << (bg.size & 1);
  int height = 1024 >> (bg.size & 1);

  AffineRenderLoop(vcount, 0, width, height, [&](int line_x, int x, int y) {
    u8 index = read<u8>(render_vram_bg, y * width + x);
    if (index == 0) {
      buffer_bg[2][line_x] = s_color_transparent;
    } else {
      buffer_bg[2][line_x] = ReadPalette(0, index);
    }
  });
}

} // namespace lunar::nds
