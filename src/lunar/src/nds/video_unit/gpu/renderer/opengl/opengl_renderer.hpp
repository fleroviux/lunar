/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunar/integer.hpp>
#include <GL/glew.h>
#include <utility>

#include "common/static_vec.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/buffer_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/program_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/shader_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/vertex_array_object.hpp"
#include "nds/video_unit/gpu/renderer/renderer_base.hpp"

namespace lunar::nds {

struct OpenGLRenderer final : RendererBase {
  OpenGLRenderer();
 ~OpenGLRenderer() override;

  void Render(void const* polygons, int polygon_count) override;

private:
  /**
   * Up to 2048 n-gons can be rendered in a single frame.
   * A single n-gon may have up to ten vertices (10-gon).
   *
   * We render each n-gon with `n - 2` triangles, meaning there are
   * up to 8 triangles per n-gon.
   *
   * Each triangle consists of three vertices.
   */
  static constexpr size_t k_total_vertices = 2048 * 8 * 3;

  struct BufferVertex {
    // clip-space position
    float x;
    float y;
    float z;
    float w;

    // vertex color
    float r;
    float g;
    float b;
    float a;
  } __attribute__((packed));

  ProgramObject* program;
  VertexArrayObject* vao;
  BufferObject* vbo;
  StaticVec<BufferVertex, k_total_vertices> vertex_buffer;
};

} // namespace lunar::nds