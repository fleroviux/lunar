/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <optional>
#include <vector>

#include "matrix.hpp"

namespace fauxDS::core {

struct GPURendererBase {
  struct Vertex {
    Vector4 position;
    s32 color[3];
    s16 uv[2];
  };

  struct Texture {
    u32 address;
    u16 palette_base;
    bool color0_transparent;

    enum class Format {
      None,
      A3I5,
      Palette2BPP,
      Palette4BPP,
      Palette8BPP,
      Compressed4x4,
      A5I3,
      Direct
    } format = Format::None;

    bool repeat[2];
    bool flip[2];
    int size[2];
  };

  virtual ~GPURendererBase() = default;

  virtual auto AddVertex(Vertex const& vertex) -> std::optional<int> = 0;
  virtual auto AddPolygon(
    std::vector<int> const& indices,
    Texture const& texture
  ) -> bool = 0;

  virtual void SwapBuffers() = 0;
  virtual void Render() = 0;
};

} // namespace fauxDS::core
