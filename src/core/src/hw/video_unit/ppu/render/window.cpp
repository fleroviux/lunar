/*
 * Copyright (C) 2020 fleroviux
 */

#include <hw/video_unit/ppu/ppu.hpp>

namespace Duality::core {

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
      for (int x = 0; x < 256; x++) {
        buffer_win[id][x] = x >= winh.min && x < winh.max;
      }
    } else {
      for (int x = 0; x < 256; x++) {
        buffer_win[id][x] = x >= winh.min || x < winh.max;
      }
    }
    
    winh._changed = false;
  }
}

} // namespace Duality::core