/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/integer.hpp>

namespace fauxDS::core {

struct BackgroundControl {
  int  priority;
  int  tile_block;
  bool enable_mosaic;
  bool full_palette;
  int  map_block;
  bool wraparound = false;
  int  ext_palette_slot = 0;
  int  size;

  BackgroundControl(int id) : id(id) {}

  void Reset();
  auto ReadByte (uint offset) -> u8;
  void WriteByte(uint offset, u8 value);

private:
  int id;
};

struct BackgroundOffset {
  u16 value;

  void Reset();
  void WriteByte(uint offset, u8 value);
};

} // namespace fauxDS::core
