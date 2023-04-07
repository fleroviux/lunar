/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <algorithm>

#include "software_renderer.hpp"

namespace lunar::nds {

auto SoftwareRenderer::SampleTexture(
  GPU::TextureParams const& params,
  Vector2<Fixed12x4> const& uv
) -> Color4 {
  const int size[2] {
    8 << params.size[0],
    8 << params.size[1]
  };

  int coord[2] { uv.X().integer(), uv.Y().integer() };

  for (int i = 0; i < 2; i++) {
    if (coord[i] < 0 || coord[i] >= size[i]) {
      int mask = size[i] - 1;
      if (params.repeat[i]) {
        bool odd = (coord[i] >> (3 + params.size[i])) & 1;
        coord[i] &= mask;
        if (params.flip[i] && odd) {
          coord[i] ^= mask;
        }
      } else {
        coord[i] = std::clamp(coord[i], 0, mask);
      }
    }
  }

  auto offset = coord[1] * size[0] + coord[0];
  auto palette_addr = params.palette_base << 4;

  switch (params.format) {
    case GPU::TextureParams::Format::None: {
      return Color4{};
    }
    case GPU::TextureParams::Format::A3I5: {
      u8  value = ReadTextureVRAM<u8>(params.address + offset);
      int index = value & 0x1F;
      int alpha = value >> 5;

      auto rgb555 = ReadPaletteVRAM<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF;
      auto rgb6666 = Color4::FromRGB555(rgb555);

      rgb6666.A() = (alpha << 3) | alpha; // 3-bit alpha to 6-bit alpha
      return rgb6666;
    }
    case GPU::TextureParams::Format::Palette2BPP: {
      auto index = (ReadTextureVRAM<u8>(params.address + (offset >> 2)) >> (2 * (offset & 3))) & 3;

      if (params.color0_transparent && index == 0) {
        return Color4{0, 0, 0, 0};
      }

      return Color4::FromRGB555(ReadPaletteVRAM<u16>((palette_addr >> 1) + index * sizeof(u16)) & 0x7FFF);
    }
    case GPU::TextureParams::Format::Palette4BPP: {
      auto index = (ReadTextureVRAM<u8>(params.address + (offset >> 1)) >> (4 * (offset & 1))) & 15;

      if (params.color0_transparent && index == 0) {
        return Color4{0, 0, 0, 0};
      }

      return Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
    }
    case GPU::TextureParams::Format::Palette8BPP: {
      auto index = ReadTextureVRAM<u8>(params.address + offset);

      if (params.color0_transparent && index == 0) {
        return Color4{0, 0, 0, 0};
      }

      return Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
    }
    case GPU::TextureParams::Format::Compressed4x4: {
      auto row_x = coord[0] >> 2;
      auto row_y = coord[1] >> 2;
      auto tile_x = coord[0] & 3;
      auto tile_y = coord[1] & 3;
      auto row_size = size[0] >> 2;

      auto data_address = params.address + (row_y * row_size + row_x) * sizeof(u32) + tile_y;

      auto data_slot_index  = data_address >> 18;
      auto data_slot_offset = data_address & 0x1FFFF;
      auto info_address = 0x20000 + (data_slot_offset >> 1) + (data_slot_index * 0x10000);

      auto data = ReadTextureVRAM<u8>(data_address);
      auto info = ReadTextureVRAM<u16>(info_address);

      auto index = (data >> (tile_x * 2)) & 3;
      auto palette_offset = info & 0x3FFF;
      auto mode = info >> 14;

      palette_addr += palette_offset << 2;

      switch (mode) {
        case 0: {
          if (index == 3) {
            return Color4{0, 0, 0, 0};
          }
          return Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
        }
        case 1: {
          if (index == 2) {
            auto color_0 = Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + 0) & 0x7FFF);
            auto color_1 = Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + 2) & 0x7FFF);

            for (uint i = 0; i < 3; i++) {
              color_0[i] = Fixed6{s8((color_0[i].raw() >> 1) + (color_1[i].raw() >> 1))};
            }

            return color_0;
          }
          if (index == 3) {
            return Color4{0, 0, 0, 0};
          }
          return Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
        }
        case 2: {
          return Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
        }
        default: {
          if (index == 2 || index == 3) {
            int coeff_0 = index == 2 ? 5 : 3;
            int coeff_1 = index == 2 ? 3 : 5;

            auto color_0 = Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + 0) & 0x7FFF);
            auto color_1 = Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + 2) & 0x7FFF);

            for (uint i = 0; i < 3; i++) {
              color_0[i] = Fixed6{s8(((color_0[i].raw() * coeff_0) + (color_1[i].raw() * coeff_1)) >> 3)};
            }

            return color_0;
          }
          return Color4::FromRGB555(ReadPaletteVRAM<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
        }
      }
    }
    case GPU::TextureParams::Format::A5I3: {
      u8  value = ReadTextureVRAM<u8>(params.address + offset);
      int index = value & 7;
      int alpha = value >> 3;

      auto rgb555 = ReadPaletteVRAM<u16>((params.palette_base << 4) + index * sizeof(u16)) & 0x7FFF;
      auto rgb6666 = Color4::FromRGB555(rgb555);

      rgb6666.A() = (alpha << 1) | (alpha >> 4); // 5-bit alpha to 6-bit alpha
      return rgb6666;
    }
    case GPU::TextureParams::Format::Direct: {
      auto rgb1555 = ReadTextureVRAM<u16>(params.address + offset * sizeof(u16));
      auto rgb6666 = Color4::FromRGB555(rgb1555);

      rgb6666.A() = rgb6666.A().raw() * (rgb1555 >> 15);
      return rgb6666;
    }
  };

  return Color4{};
}

} // namespace lunar::nds
