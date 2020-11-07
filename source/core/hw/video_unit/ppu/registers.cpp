/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "registers.hpp"

namespace fauxDS::core {

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
             (ext_palette_slot << 5) |
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
        ext_palette_slot = (value >> 5) & 1;
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
