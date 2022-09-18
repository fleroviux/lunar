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
}

auto TextureCache::Get(void const* params_) -> GLuint {
  auto params = (GPU::TextureParams*)params_;

  if (auto match = cache.find(params->raw_value); match != cache.end()) {
    return match->second;
  }

  const int width  = 8 << params->size[0];
  const int height = 8 << params->size[1];

  const GPU::TextureParams::Format format = params->format;

  GLuint texture;

  // TODO: configure wrap behaviour
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  u32* data = new u32[width * height];

  switch (format) {
    case GPU::TextureParams::Format::A3I5: {
      Decode_A3I5(width, height, params, data);
      break;
    }
    case GPU::TextureParams::Format::A5I3: {
      Decode_A5I3(width, height, params, data);
      break;
    }
    case GPU::TextureParams::Format::Palette2BPP: {
      Decode_Palette2BPP(width, height, params, data);
      break;
    }
    case GPU::TextureParams::Format::Palette4BPP: {
      Decode_Palette4BPP(width, height, params, data);
      break;
    }
    case GPU::TextureParams::Format::Palette8BPP: {
      Decode_Palette8BPP(width, height, params, data);
      break;
    }
    case GPU::TextureParams::Format::Compressed4x4: {
      Decode_Compressed4x4(width, height, params, data);
      break;
    }
    default: {
      fmt::print("texture cache: unsupported format: {}\n", (int)format);

      for (int i = 0; i < width * height; i++) {
        data[i] = 0xFFFF00FF; // missing texture pink
      }
      break;
    }
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
  delete[] data;

  for (int i : {0, 1}) {
    constexpr GLenum parameter[2] {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T};

    GLint mode = GL_CLAMP_TO_EDGE;

    if (params->repeat[i]) {
      mode = params->flip[i] ? GL_MIRRORED_REPEAT : GL_REPEAT;
    }

    glTexParameteri(GL_TEXTURE_2D, parameter[i], mode);
  }

  // TODO: we probably don't want to include all bits in the cache key.
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

  // TODO: think about fetching 32 pixels (64-bits) at once
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

  // TODO: think about fetching 16 pixels (64-bits) at once
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

  // TODO: think about fetching 8 pixels (64-bits) at once
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

      for (int inner_y = 0; inner_y < 4; inner_y++) {
        for(int inner_x = 0; inner_x < 4; inner_x++) {
          uint index = block_data & 3;

          // TODO: handle blend modes and transparency
          u16 color = vram_palette.Read<u16>(palette_address + palette_offset * sizeof(u32) + index * sizeof(u16)) & 0x7FFF;

          data[(block_y + inner_y) * width + block_x + inner_x] = ConvertColor(color);

          block_data >>= 2;
        }
      }
    }
  }
}

} // namespace lunar::nds