/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include "hw/video_unit/gpu/renderer_base.hpp"

namespace Duality::core {

struct GPUSoftwareRenderer final : GPURendererBase {
  auto AddVertex(Vertex const& vertex) -> std::optional<int> override { return {}; }
  auto AddPolygon(
    std::vector<int> const& indices,
    Texture const& texture
  ) -> bool override { return false; }

  void SwapBuffers() override { }
  void Render() override { }
};

} // namespace Duality::core
