/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <GL/glew.h>

#include "nds/video_unit/gpu/renderer/renderer_base.hpp"

namespace lunar::nds {

struct OpenGLRenderer final : RendererBase {
  OpenGLRenderer();
 ~OpenGLRenderer();

  void Render(void const* polygons, int polygon_count) override;
};

} // namespace lunar::nds