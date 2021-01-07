/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <core/hw/video_unit/gpu/renderer_base.hpp>

namespace fauxDS::core {

struct GPUSoftwareRenderer final : GPURendererBase {
  auto AddVertex(Vertex const& vertex) -> std::optional<int> override { return {}; }
  auto AddPolygon(
    std::vector<int> const& indices,
    Texture const& texture
  )-> bool override { return false; }

  void SwapBuffers() override { }
  void Render() override { }
};

} // namespace fauxDS::core
