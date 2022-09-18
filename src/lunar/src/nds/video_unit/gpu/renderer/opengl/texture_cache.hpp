/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <GL/glew.h>
#include <unordered_map>

#include "nds/video_unit/vram_region.hpp"

namespace lunar::nds {

struct TextureCache {
  TextureCache(
    Region<4, 131072> const& vram_texture,
    Region<8> const& vram_palette
  );

  auto Get(void const* params) -> GLuint;

private:
  void Decode_Palette4BPP(int width, int height, void const* params, u32* data);

  Region<4, 131072> const& vram_texture;
  Region<8> const& vram_palette;

  std::unordered_map<u32, GLuint> cache;
};

} // namespace lunar::nds