/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "registers.hpp"

namespace fauxDS::core {

void DisplayControl::Reset() {
}

auto DisplayControl::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0: {
      return bg_mode |
             (enable_bg0_3d ? 8 : 0) |
             (static_cast<u8>(tile_obj.mapping) << 4) |
             (bitmap_obj.dimension << 5) |
             (static_cast<u8>(bitmap_obj.mapping) << 6) |
             (forced_blank ? 128 : 0);
    }
    case 1: {
      u8 value = 0;
      for (int i = 0; i < 8; i++)
        value |= enable[i] ? (1 << i) : 0;
      return value;
    }
    case 2: {
      return display_mode |
            (vram_block << 2) |
            (tile_obj.boundary << 4) |
            (bitmap_obj.boundary << 6) |
            (hblank_oam_update ? 128 : 0);
    }
    case 3: {
      return tile_block |
             (map_block << 3) |
             (enable_ext_pal_bg ? 64 : 0) |
             (enable_ext_pal_obj ? 128 : 0);
    }
  }

  UNREACHABLE;
}

void DisplayControl::WriteByte(uint offset, u8 value) {
  value &= mask >> (offset * 8);

  switch (offset) {
    case 0:
      bg_mode = value & 7;
      enable_bg0_3d = value & 8;
      tile_obj.mapping = static_cast<Mapping>((value >> 4) & 1);
      bitmap_obj.dimension = (value >> 5) & 1;
      bitmap_obj.mapping = static_cast<Mapping>((value >> 6) & 1);
      forced_blank = value & 128;
      break;
    case 1:
      for (int i = 0; i < 8; i++)
        enable[i] = value & (1 << i);
      break;
    case 2:
      display_mode = value & 3;
      vram_block = (value >> 2) & 3;
      tile_obj.boundary = (value >> 4) & 3;
      bitmap_obj.boundary = (value >> 6) & 1;
      hblank_oam_update = value & 128;
      break;
    case 3:
      tile_block = value & 7;
      map_block = (value >> 3) & 7;
      enable_ext_pal_bg = value & 64;
      enable_ext_pal_obj = value & 128; 
      break;
    default:
      UNREACHABLE;
  }
}

void BackgroundControl::Reset() {
  WriteByte(0, 0);
  WriteByte(1, 0);
}

auto BackgroundControl::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return priority |
             (tile_block << 2) |
             (enable_mosaic ? 64 : 0) |
             (full_palette ? 128 : 0);
    case 1:
      return map_block |
             (palette_slot << 5) |
             (wraparound ? 32 : 0) |
             (size << 6);
  }

  UNREACHABLE;
}

void BackgroundControl::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      priority = value & 3;
      tile_block = (value >> 2) & 15;
      enable_mosaic = value & 64;
      full_palette = value & 128;
      break;
    case 1:
      map_block = value & 0x1F;
      if (id <= 1) {
        palette_slot = (value >> 5) & 1;
      } else {
        wraparound = value & 32;
      }
      size = value >> 6;
      break;
    default:
      UNREACHABLE;
  }
}

void BackgroundOffset::Reset() {
  WriteByte(0, 0);
  WriteByte(1, 0);
}

void BackgroundOffset::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      this->value = (this->value & ~0xFF) |  value;
      break;
    case 1:
      this->value = (this->value &  0xFF) | ((value & 1) << 8);
      break;
    default:
      UNREACHABLE;
  }
}

} // namespace fauxDS::core
