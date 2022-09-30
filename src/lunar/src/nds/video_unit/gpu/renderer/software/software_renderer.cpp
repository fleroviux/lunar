/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "software_renderer.hpp"

namespace lunar::nds {

SoftwareRenderer::SoftwareRenderer(
  Region<4, 131072> const& vram_texture,
  Region<8> const& vram_palette
) : vram_texture(vram_texture), vram_palette(vram_palette) {
  std::memset(vram_texture_copy, 0, sizeof(vram_texture_copy));
  std::memset(vram_palette_copy, 0, sizeof(vram_palette_copy));
}

} // namespace lunar::nds
