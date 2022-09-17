/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

namespace lunar::nds {

// TODO: come up with a way to nicely share definitions between the GPU and renderer.

struct RendererBase {
  virtual ~RendererBase() = default;

  virtual void Render(void const* polygons, int polygon_count) = 0;
};

} // namespace lunar::nds