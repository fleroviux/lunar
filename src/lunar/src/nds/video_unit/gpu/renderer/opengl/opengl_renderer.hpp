/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <array>
#include <lunar/integer.hpp>
#include <GL/glew.h>
#include <utility>

// Include gpu.hpp for definitions... (need to find a better solution for this)
#include "nds/video_unit/gpu/gpu.hpp"

#include "common/static_vec.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/buffer_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/frame_buffer_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/program_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/shader_object.hpp"
#include "nds/video_unit/gpu/renderer/opengl/hal/texture_2d.hpp"
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
    GPU::AlphaTest const& alpha_test,
    GPU::FogColor const& fog_color,
    GPU::FogOffset const& fog_offset,
    std::array<u16, 8> const& edge_color_table
  );

 ~OpenGLRenderer() override;

  void Render(void const** polygons, int polygon_count) override;
  void UpdateToonTable(std::array<u16, 32> const& toon_table) override;
  void UpdateFogDensityTable(std::array<u8, 32> const& fog_density_table) override;
  void SetWBufferEnable(bool enable) override;

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

    // This flag can be computed from existing render state,
    // it is just kept here for convenience.
    bool translucent;

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
  void RenderPolygons(void const** polygons, int polygon_count);
  void RenderEdgeMarking();
  void RenderFog();
  void SetupAndUploadVBO(void const** polygons, int polygon_count);

  // FBO
  FrameBufferObject* fbo;
  Texture2D* color_texture;
  Texture2D* opaque_poly_id_texture;
  Texture2D* depth_texture;

  // Geoemtry render pass
  ProgramObject* program;
  VertexArrayObject* vao;
  BufferObject* vbo;
  StaticVec<BufferVertex, k_total_vertices> vertex_buffer;
  // @todo: make sure that 2048 *really* is enough.
  StaticVec<Batch, 2048> batch_list;

  // Edge-marking pass
  FrameBufferObject* fbo_edge_marking;
  ProgramObject* program_edge_marking;
  VertexArrayObject* quad_vao;
  BufferObject* quad_vbo;

  // Fog pass
  FrameBufferObject* fbo_fog;
  Texture2D* fog_output_texture;
  ProgramObject* program_fog;
  Texture2D* fog_density_table_texture;

  Texture2D* toon_table_texture;

  TextureCache texture_cache;

  // MMIO passed through from the GPU:
  GPU::DISP3DCNT const& disp3dcnt;
  GPU::AlphaTest const& alpha_test;
  GPU::FogColor const& fog_color;
  GPU::FogOffset const& fog_offset;
  std::array<u16, 8> const& edge_color_table;

  bool use_w_buffer = false;
};

} // namespace lunar::nds