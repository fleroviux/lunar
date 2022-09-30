/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

// Include gpu.hpp for definitions... (need to find a better solution for this)
#include "nds/video_unit/gpu/gpu.hpp"

// @todo: move color.hpp to the software renderer (it is not used outside afaik)

#include "common/punning.hpp"
#include "nds/video_unit/gpu/renderer/renderer_base.hpp"
#include "nds/video_unit/gpu/color.hpp"
#include "nds/video_unit/vram_region.hpp"

namespace lunar::nds {

struct SoftwareRenderer final : RendererBase {
  SoftwareRenderer(
    Region<4, 131072> const& vram_texture,
    Region<8> const& vram_palette
  );

  auto GetOutput() -> void const* override {
    return nullptr;
  }

  auto GetOutputImageType() const -> VideoDevice::ImageType override {
    return VideoDevice::ImageType::Software;
  }

  void Render(void const** polygons, int polygon_count) override {
  }

  void UpdateToonTable(std::array<u16, 32> const& toon_table) override {
  }

  void UpdateFogDensityTable(std::array<u8, 32> const& fog_density_table) override {
  }

  void SetWBufferEnable(bool enable) override {
  }

  void CaptureColor(u16* buffer, int vcount, int width, bool display_capture) override {
  }

  void CaptureAlpha(int* buffer, int vcount) override {
  }

private:
  template<typename T>
  auto ReadTextureVRAM(u32 address) {
    return read<T>(vram_texture_copy, address & 0x7FFFF & ~(sizeof(T) - 1));
  }

  template<typename T>
  auto ReadPaletteVRAM(u32 address) {
    return read<T>(vram_palette_copy, address & 0x1FFFF & ~(sizeof(T) - 1));
  }

  auto SampleTexture(
    GPU::TextureParams const& params,
    Vector2<Fixed12x4> const& uv
  ) -> Color4;

  /// GPU texture and texture palette data
  Region<4, 131072> const& vram_texture { 3 };
  Region<8> const& vram_palette { 7 };
  u8 vram_texture_copy[524288];
  u8 vram_palette_copy[131072];
};

} // namespace lunar::nds
