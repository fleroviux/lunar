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

// Include gpu.hpp for definitions... (need to find a better solution for this)
#include "nds/video_unit/gpu/gpu.hpp"

#include "common/static_vec.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/buffer_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/program_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/shader_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/vertex_array_object.hpp"
#include "nds/video_unit/gpu/renderer/renderer_base.hpp"
#include "nds/video_unit/vram_region.hpp"
#include "texture_cache.hpp"

namespace lunar::nds {

struct OpenGLRenderer final : RendererBase {
  OpenGLRenderer(
    Region<4, 131072> const& vram_texture,
    Region<8> const& vram_palette,
    GPU::DISP3DCNT const& disp3dcnt,
    GPU::AlphaTest const& alpha_test
  );

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

  enum StencilBufferBits {
    STENCIL_MASK_POLY_ID = 31,
    STENCIL_FLAG_SHADOW = 128
  };

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

    // texture coordinate
    float s;
    float t;
  } __attribute__((packed));

  struct RenderState {
    void const* texture_params;
    int alpha;
    bool enable_translucent_depth_write;
    int polygon_id;
    int polygon_mode; // @todo: use correct data type
    int depth_test; // @todo: use correct data type

    bool operator==(RenderState const& other) const;

    bool operator!=(RenderState const& other) const {
      return !(*this == other);
    }
  };

  struct Batch {
    RenderState state{};
    int vertex_start = 0;
    int vertex_count = 0;
  };

  void RenderRearPlane();
  void RenderPolygons(void const* polygons, int polygon_count, bool translucent);
  void SetupAndUploadVBO(void const* polygons, int polygon_count);

  // FBO
  GLuint fbo;
  GLuint color_texture;
  GLuint depth_texture;

  ProgramObject* program;
  VertexArrayObject* vao;
  BufferObject* vbo;
  StaticVec<BufferVertex, k_total_vertices> vertex_buffer;

  // @todo: make sure that 2048 *really* is enough.
  StaticVec<Batch, 2048> batch_list;

  TextureCache texture_cache;

  // MMIO passed through from the GPU:
  GPU::DISP3DCNT const& disp3dcnt;
  GPU::AlphaTest const& alpha_test;
};

} // namespace lunar::nds