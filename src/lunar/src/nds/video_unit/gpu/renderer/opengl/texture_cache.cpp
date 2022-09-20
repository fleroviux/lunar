/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Include gpu.hpp for definitions... (need to find a better solution for this)
#include "nds/video_unit/gpu/gpu.hpp"

#include "texture_cache.hpp"

namespace lunar::nds {

TextureCache::TextureCache(
  Region<4, 131072> const& vram_texture,
  Region<8> const& vram_palette
)   : vram_texture(vram_texture), vram_palette(vram_palette) {
  RegisterVRAMMapUnmapHandlers();
}

auto TextureCache::Get(void const* params_) -> Texture2D* {
  using Format = GPU::TextureParams::Format;

  auto params = (GPU::TextureParams*)params_;

  if (auto match = cache.find(params->raw_value); match != cache.end()) {
    return match->second;
  }

  const int width  = 8 << params->size[0];
  const int height = 8 << params->size[1];

  const GPU::TextureParams::Format format = params->format;

  u32* data = new u32[width * height];

  switch (format) {
    case Format::None: break;
    case Format::A3I5: Decode_A3I5(width, height, params, data); break;
    case Format::A5I3: Decode_A5I3(width, height, params, data); break;
    case Format::Palette2BPP:   Decode_Palette2BPP(width, height, params, data); break;
    case Format::Palette4BPP:   Decode_Palette4BPP(width, height, params, data); break;
    case Format::Palette8BPP:   Decode_Palette8BPP(width, height, params, data); break;
    case Format::Compressed4x4: Decode_Compressed4x4(width, height, params, data); break;
    case Format::Direct: Decode_Direct(width, height, params, data); break;
  }

  Texture2D* texture = Texture2D::Create(width, height, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE, data);
  texture->SetMinFilter(GL_LINEAR);
  texture->SetMagFilter(GL_LINEAR);

  delete[] data;

  const GLint wrap_s = params->repeat[0] ?
    (params->flip[0] ? GL_MIRRORED_REPEAT : GL_REPEAT) : GL_CLAMP_TO_EDGE;

  const GLint wrap_t = params->repeat[1] ?
    (params->flip[1] ? GL_MIRRORED_REPEAT : GL_REPEAT) : GL_CLAMP_TO_EDGE;

  texture->SetWrapBehaviour(wrap_s, wrap_t);

  // @todo: we probably don't want to include all bits in the cache key.
  cache[params->raw_value] = texture;
  return texture;
}

void TextureCache::Decode_A3I5(int width, int height, const void *params_, u32 *data) {
  auto params = (GPU::TextureParams*)params_;
  int texels  = width * height;
  u32 texture_address = params->address;
  u32 palette_address = params->palette_base << 4;

  for (int i = 0; i < texels; i++) {
    u8 texel = vram_texture.Read<u8>(texture_address);
    int index = texel & 31;
    int alpha = texel >> 5;
    u16 rgb555 = vram_palette.Read<u16>(palette_address + index * sizeof(u16)) & 0x7FFF;

    // 3-bit alpha to 8-bit alpha conversion
    alpha = (alpha << 5) | (alpha << 2) | (alpha >> 1);

    *data++ = ConvertColor(rgb555, alpha);

    texture_address++;
  }
}

void TextureCache::Decode_A5I3(int width, int height, const void *params_, u32 *data) {
  auto params = (GPU::TextureParams*)params_;
  int texels  = width * height;
  u32 texture_address = params->address;
  u32 palette_address = params->palette_base << 4;

  for (int i = 0; i < texels; i++) {
    u8 texel = vram_texture.Read<u8>(texture_address);
    int index = texel & 7;
    int alpha = texel >> 3;
    u16 rgb555 = vram_palette.Read<u16>(palette_address + index * sizeof(u16)) & 0x7FFF;

    // 5-bit alpha to 8-bit alpha conversion
    alpha = (alpha << 3) | (alpha >> 2);

    *data++ = ConvertColor(rgb555, alpha);

    texture_address++;
  }
}

void TextureCache::Decode_Palette2BPP(int width, int height, void const* params_, u32* data) {
  auto params = (GPU::TextureParams*)params_;
  int texels  = width * height;
  u32 texture_address = params->address;
  u32 palette_address = params->palette_base << 3;
  bool color0_transparent = params->color0_transparent;

  // @todo: think about fetching 32 pixels (64-bits) at once
  for (int i = 0; i < texels; i += 4) {
    u8 indices = vram_texture.Read<u8>(texture_address);

    for (int j = 0; j < 4; j++) {
      int index = indices & 2;

      // @todo: do 2BPP textures actually honor the color0_transparent flag?
      if (color0_transparent && index == 0) {
        *data++ = 0;
      } else {
        *data++ = ConvertColor(vram_palette.Read<u16>(palette_address + index * sizeof(u16)) & 0x7FFF);
      }

      indices >>= 2;
    }

    texture_address++;
  }
}

void TextureCache::Decode_Palette4BPP(int width, int height, void const* params_, u32* data) {
  auto params = (GPU::TextureParams*)params_;
  int texels  = width * height;
  u32 texture_address = params->address;
  u32 palette_address = params->palette_base << 4;
  bool color0_transparent = params->color0_transparent;

  // @todo: think about fetching 16 pixels (64-bits) at once
  for (int i = 0; i < texels; i += 2) {
    u8 indices = vram_texture.Read<u8>(texture_address);

    for (int j = 0; j < 2; j++) {
      int index = indices & 15;

      if (color0_transparent && index == 0) {
        *data++ = 0;
      } else {
        *data++ = ConvertColor(vram_palette.Read<u16>(palette_address + index * sizeof(u16)) & 0x7FFF);
      }

      indices >>= 4;
    }

    texture_address++;
  }
}

void TextureCache::Decode_Palette8BPP(int width, int height, void const* params_, u32* data) {
  auto params = (GPU::TextureParams*)params_;
  int texels  = width * height;
  u32 texture_address = params->address;
  u32 palette_address = params->palette_base << 4;
  bool color0_transparent = params->color0_transparent;

  // @todo: think about fetching 8 pixels (64-bits) at once
  for (int i = 0; i < texels; i++) {
    u8 index = vram_texture.Read<u8>(texture_address);

    if (color0_transparent && index == 0) {
      *data++ = 0;
    } else {
      *data++ = ConvertColor(vram_palette.Read<u16>(palette_address + index * sizeof(u16)) & 0x7FFF);
    }

    texture_address++;
  }
}

void TextureCache::Decode_Compressed4x4(int width, int height, void const* params_, u32* data) {
  auto params = (GPU::TextureParams*)params_;
  u32 texture_address = params->address;
  u32 palette_address = params->palette_base << 4;

  int rows = width >> 2;

  for (int block_y = 0; block_y < height; block_y += 4) {
    for (int block_x = 0; block_x < width; block_x += 4) {
      u32 block_data_address = texture_address + block_y * rows + block_x;

      u32 block_data_slot_index  = block_data_address >> 18;
      u32 block_data_slot_offset = block_data_address & 0x1FFFF;
      u32 block_info_address = 0x20000 + (block_data_slot_offset >> 1) + (block_data_slot_index * 0x10000);

      u32 block_data = vram_texture.Read<u32>(block_data_address);
      u16 block_info = vram_texture.Read<u16>(block_info_address);

      // decode block information
      int palette_offset = block_info & 0x3FFF;
      int blend_mode = block_info >> 14;

      u32 final_palette_address = palette_address + palette_offset * sizeof(u32);

      const auto FetchPRAM = [&](uint index) {
        return ConvertColor(vram_palette.Read<u16>(final_palette_address + index * sizeof(u16)) & 0x7FFF);
      };

      const auto Mix = [&](u32 color0, u32 color1, int alpha0, int alpha1, int shift) -> u32 {
        int r0 = (int)(color0 >> 16) & 0xFF;
        int g0 = (int)(color0 >>  8) & 0xFF;
        int b0 = (int)(color0 >>  0) & 0xFF;

        int r1 = (int)(color1 >> 16) & 0xFF;
        int g1 = (int)(color1 >>  8) & 0xFF;
        int b1 = (int)(color1 >>  0) & 0xFF;

        int ro = (r0 * alpha0 + r1 * alpha1) >> shift;
        int go = (g0 * alpha0 + g1 * alpha1) >> shift;
        int bo = (b0 * alpha0 + b1 * alpha1) >> shift;

        return 0xFF000000 | ro << 16 | go << 8 | bo;
      };

      u32 palette[4];

      palette[0] = FetchPRAM(0);
      palette[1] = FetchPRAM(1);

      switch (blend_mode) {
        case 0: {
          palette[2] = FetchPRAM(2);
          palette[3] = 0;
          break;
        }
        case 1: {
          palette[2] = Mix(palette[0], palette[1], 1, 1, 1);
          palette[3] = 0;
          break;
        }
        case 2: {
          palette[2] = FetchPRAM(2);
          palette[3] = FetchPRAM(3);
          break;
        }
        case 3: {
          palette[2] = Mix(palette[0], palette[1], 5, 3, 3);
          palette[3] = Mix(palette[0], palette[1], 3, 5, 3);
          break;
        }
      }

      for (int inner_y = 0; inner_y < 4; inner_y++) {
        u32* row_data = &data[(block_y + inner_y) * width + block_x];

        for(int inner_x = 0; inner_x < 4; inner_x++) {
          *row_data++ = palette[block_data & 3];
          block_data >>= 2;
        }
      }
    }
  }
}

void TextureCache::Decode_Direct(int width, int height, void const* params_, u32* data) {
  auto params = (GPU::TextureParams*)params_;
  int texels  = width * height;
  u32 texture_address = params->address;

  for (int i = 0; i < texels; i++) {
    u16 abgr1555 = vram_texture.Read<u16>(texture_address);

    *data++ = ConvertColor(abgr1555, (abgr1555 >> 15) * 255);
    texture_address += sizeof(u16);
  }
}

void TextureCache::RegisterVRAMMapUnmapHandlers() {
  const auto OnMapUnmap = [&](u32 offset, size_t size) {
    // this is the poor girl's cache invalidation.
    // @todo: do not invalidate textures which aren't affected.
    for (auto& entry : cache) {
      delete entry.second;
    }

    cache = {};
  };

  vram_texture.AddCallback(OnMapUnmap);
  vram_palette.AddCallback(OnMapUnmap);
}

} // namespace lunar::nds