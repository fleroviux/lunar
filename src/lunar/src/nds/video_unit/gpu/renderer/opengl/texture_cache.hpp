/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <GL/glew.h>
#include <unordered_map>

#include "common/ogl/texture_2d.hpp"
#include "nds/video_unit/vram_region.hpp"

namespace lunar::nds {

class TextureCache {
  public:
    TextureCache(
      Region<4, 131072> const& vram_texture,
      Region<8> const& vram_palette
    );

    auto Get(void const* params) -> Texture2D*;

  private:
    // @fixme: this more or less is a copy from ppu.hpp:
    static auto ConvertColor(u16 color, u8 alpha = 0xFF) -> u32 {
      u32 r = (color >>  0) & 0x1F;
      u32 g = (color >>  5) & 0x1F;
      u32 b = (color >> 10) & 0x1F;

      return r << 19 | g << 11 | b << 3 | (alpha << 24);
    }

    void Decode_A3I5(int width, int height, void const* params, u32* data);
    void Decode_A5I3(int width, int height, void const* params, u32* data);
    void Decode_Palette2BPP(int width, int height, void const* params, u32* data);
    void Decode_Palette4BPP(int width, int height, void const* params, u32* data);
    void Decode_Palette8BPP(int width, int height, void const* params, u32* data);
    void Decode_Compressed4x4(int width, int height, void const* params, u32* data);
    void Decode_Direct(int width, int height, void const* params, u32* data);

    void RegisterVRAMMapUnmapHandlers();

    Region<4, 131072> const& vram_texture;
    Region<8> const& vram_palette;

    std::unordered_map<u64, Texture2D*> cache;
};

} // namespace lunar::nds