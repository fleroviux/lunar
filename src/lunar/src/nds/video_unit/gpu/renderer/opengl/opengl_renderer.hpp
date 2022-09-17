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
#include "nds/video_unit/gpu/renderer/renderer_base.hpp"

namespace lunar::nds {

struct OpenGLRenderer final : RendererBase {
  OpenGLRenderer();
 ~OpenGLRenderer();

  void Render(void const* polygons, int polygon_count) override;

private:
  // max_polygons * max_triangles_per_polygon * vertices_per_polygon
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

  StaticVec<BufferVertex, k_total_vertices> vertex_buffer;

  GLuint vao;
  GLuint vbo;

  // ---------------------------------------

  auto CompileShader(
    GLenum type,
    char const* source
  ) -> std::pair<bool, GLuint>;

  auto CompileProgram(
    char const* vertex_src,
    char const* fragment_src
  ) -> std::pair<bool, GLuint>;

  GLuint test_program;
};

} // namespace lunar::nds