/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/integer.hpp>

namespace fauxDS::core {

struct DisplayControl {
  enum class Mapping {
    TwoDimensional = 0,
    OneDimensional = 1
  };

  int  bg_mode = 0;
  bool enable_bg0_3d = false;
  bool forced_blank = false;
  bool enable[8] {false};
  int  display_mode = 0;
  int  vram_block = 0;
  bool hblank_oam_update = false;
  int  tile_block = 0;
  int  map_block = 0;
  bool enable_ext_pal_bg = false;
  bool enable_ext_pal_obj = false;

  struct {
    Mapping mapping = Mapping::TwoDimensional;
    int boundary = 0;
  } tile_obj;

  struct {
    Mapping mapping = Mapping::TwoDimensional;
    int dimension = 0;
    int boundary = 0;
  } bitmap_obj;

  DisplayControl(u32 mask = 0xFFFFFFFF) : mask(mask) {}

  void Reset();
  auto ReadByte (uint offset) -> u8;
  void WriteByte(uint offset, u8 value);

private:
  u32 mask;
};

struct BackgroundControl {
  int  priority;
  int  tile_block;
  bool enable_mosaic;
  bool full_palette;
  int  map_block;
  bool wraparound = false;
  int  palette_slot = 0;
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

struct ReferencePoint {
  s32 initial;
  s32 _current;
  
  void Reset();
  void WriteByte(uint offset, u8 value);
};

struct RotateScaleParameter {
  s16 value;

  void Reset();
  void WriteByte(uint offset, u8 value);
};

struct WindowRange {
  u8 min;
  u8 max;
  bool _changed;

  void Reset();
  void WriteByte(uint offset, u8 value);
};

struct WindowLayerSelect {
  bool enable[2][6];

  void Reset();
  auto ReadByte(uint offset) -> u8;
  void WriteByte(uint offset, u8 value);
};

struct BlendControl {
  enum Effect {
    SFX_NONE,
    SFX_BLEND,
    SFX_BRIGHTEN,
    SFX_DARKEN
  } sfx;
  
  bool targets[2][6];

  void Reset();
  auto ReadByte(uint offset) -> u8;
  void WriteByte(uint offset, u8 value);
};

struct BlendAlpha {
  int a;
  int b;

  void Reset();
  auto ReadByte(uint offset) -> u8;
  void WriteByte(uint offset, u8 value);
};

struct BlendBrightness {
  int y;

  void Reset();
  void WriteByte(uint offset, u8 value);
};

struct Mosaic {
  struct {
    int size_x;
    int size_y;
    int _counter_y;
  } bg, obj;
  
  void Reset();
  void WriteByte(uint offset, u8 value);
};

} // namespace fauxDS::core
