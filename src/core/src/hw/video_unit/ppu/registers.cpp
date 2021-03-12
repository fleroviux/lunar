/*
 * Copyright (C) 2020 fleroviux
 */

#include <util/log.hpp>

#include "registers.hpp"

namespace Duality::Core {

void DisplayControl::Reset() {
  WriteByte(0, 0);
  WriteByte(1, 0);
  WriteByte(2, 0);
  WriteByte(3, 0);
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
             (enable_extpal_bg ? 64 : 0) |
             (enable_extpal_obj ? 128 : 0);
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
      enable_extpal_bg = value & 64;
      enable_extpal_obj = value & 128; 
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

void ReferencePoint::Reset() {
  initial = 0;
  _current = 0;
}

void ReferencePoint::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      initial = (initial & 0x0FFFFF00) | value;
      break;
    case 1:
      initial = (initial & 0x0FFF00FF) | (value << 8);
      break;
    case 2:
      initial = (initial & 0x0F00FFFF) | (value << 16);
      break;
    case 3:
      initial = (initial & 0x00FFFFFF) | (value << 24);
      break;
    default:
      UNREACHABLE;
  }
  
  if (initial & (1 << 27)) {
    initial |= 0xF0000000;
  }
  
  _current = initial;
}

void RotateScaleParameter::Reset() {
  value = 0;
}

void RotateScaleParameter::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      this->value = (this->value & 0xFF00) | value;
      break;
    case 1:
      this->value = (this->value & 0x00FF) | (value << 8);
      break;
    default:
      UNREACHABLE;
  }
}

void WindowRange::Reset() {
  min = 0;
  max = 0;
  _changed = false;
}

void WindowRange::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      if (value != max)
        _changed = true;
      max = value;
      break;
    case 1:
      if (value != min)
        _changed = true;
      min = value;
      break;
    default:
      UNREACHABLE;
  }
}

void WindowLayerSelect::Reset() {
  WriteByte(0, 0);
  WriteByte(1, 0);
}

auto WindowLayerSelect::ReadByte(uint offset) -> u8 {
  u8 value = 0;
  
  for (int i = 0; i < 6; i++) {
    value |= enable[offset][i] ? (1 << i) : 0;
  }
  
  return value;
}

void WindowLayerSelect::WriteByte(uint offset, u8 value) {
  for (int i = 0; i < 6; i++) {
    enable[offset][i] = value & (1 << i);
  }
}

void BlendControl::Reset() {
  WriteByte(0, 0);
  WriteByte(1, 0);
}

auto BlendControl::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0: {
      u8 value = 0;
      for (int i = 0; i < 6; i++)
        value |= targets[0][i] ? (1 << i) : 0;
      value |= static_cast<u8>(sfx << 6);
      return value;
    }
    case 1: {
      u8 value = 0;
      for (int i = 0; i < 6; i++)
        value |= targets[1][i] ? (1 << i) : 0;
      return value;
    }
  }

  UNREACHABLE;
}

void BlendControl::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      for (int i = 0; i < 6; i++)
        targets[0][i] = value & (1 << i);
      sfx = static_cast<Effect>(value >> 6);
      break;
    case 1:
      for (int i = 0; i < 6; i++)
        targets[1][i] = value & (1 << i);
      break;
    default:
      UNREACHABLE;
  }
}

void BlendAlpha::Reset() {
  WriteByte(0, 0);
  WriteByte(1, 0);
}

auto BlendAlpha::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return a;
    case 1:
      return b;
    default:
      UNREACHABLE;
  }

  return 0;
}

void BlendAlpha::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      a = value & 31;
      break;
    case 1:
      b = value & 31;
      break;
    default:
      UNREACHABLE;
  }
}

void BlendBrightness::Reset() {
  WriteByte(0, 0);
}

void BlendBrightness::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      y = value & 31;
      break;
    case 1:
    case 2:
    case 3:
      break;
    default:
      UNREACHABLE;
  }
}

void Mosaic::Reset() {
  bg.size_x = 1;
  bg.size_y = 1;
  bg._counter_y = 0;
  obj.size_x = 1;
  obj.size_y = 1;
  bg._counter_y = 0;
}

void Mosaic::WriteByte(uint offset, u8 value) {
  // TODO: how does hardware handle mid-frame or mid-line mosaic changes?
  switch (offset) {
    case 0:
      bg.size_x = (value & 15) + 1;
      bg.size_y = (value >> 4) + 1;    
      bg._counter_y = 0;
      break;
    case 1:
      obj.size_x = (value & 15) + 1;
      obj.size_y = (value >> 4) + 1;
      obj._counter_y = 0;
      break;
    default:
      UNREACHABLE;
  }
}

} // namespace Duality::Core
