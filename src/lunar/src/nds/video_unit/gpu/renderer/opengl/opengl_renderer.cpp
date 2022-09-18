/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <optional>

#include "shader/test.glsl.hpp"
#include "opengl_renderer.hpp"

#include <SDL.h>
extern SDL_Window* g_window;

namespace lunar::nds {

OpenGLRenderer::OpenGLRenderer(
  Region<4, 131072> const& vram_texture,
  Region<8> const& vram_palette,
  GPU::DISP3DCNT const& disp3dcnt,
  GPU::AlphaTest const& alpha_test
)   : texture_cache{vram_texture, vram_palette}, disp3dcnt{disp3dcnt}, alpha_test{alpha_test} {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  program = ProgramObject::Create(test_vert, test_frag);

  vbo = BufferObject::CreateArrayBuffer(sizeof(BufferVertex) * k_total_vertices, GL_DYNAMIC_DRAW);
  vao = VertexArrayObject::Create();
  // TODO: we can probably set stride to zero since the VBO is tightly packed.
  vao->SetAttribute(0, VertexArrayObject::Attribute{vbo, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 0});
  vao->SetAttribute(1, VertexArrayObject::Attribute{vbo, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 4 * sizeof(float)});
  vao->SetAttribute(2, VertexArrayObject::Attribute{vbo, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 8 * sizeof(float)});
}

OpenGLRenderer::~OpenGLRenderer() {
  delete program;
  delete vao;
  delete vbo;
}

void OpenGLRenderer::Render(void const* polygons_, int polygon_count) {
  auto polygons = (GPU::Polygon const*)polygons_;

  SetupAndUploadVBO(polygons, polygon_count);

  glViewport(0, 384, 512, 384);
  RenderRearPlane();
  RenderPolygons(polygons, polygon_count, false);
  RenderPolygons(polygons, polygon_count, true);

  SDL_GL_SwapWindow(g_window);
}

void OpenGLRenderer::RenderRearPlane() {
  // @todo: replace this stub with a real implementation
  glDepthMask(GL_TRUE);
  glClearColor(0.01, 0.01, 0.01, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::RenderPolygons(void const* polygons_, int polygon_count, bool translucent) {
  using Format = GPU::TextureParams::Format;

  auto polygons = (GPU::Polygon const*)polygons_;

  program->Use();
  vao->Bind();
  glDepthFunc(GL_LEQUAL); // TODO: set this according to polygon parameters

  if (disp3dcnt.enable_alpha_blend) {
    glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
  } else {
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
  }

  // @todo: could use different VBOs for opaque and translucent.

  Batch current_batch;
  int next_batch_start = 0;

  batch_list.clear();

  const auto FinalizeBatch = [&]() {
    current_batch.vertex_count = next_batch_start - current_batch.vertex_start;
    if (current_batch.vertex_count != 0) {
      batch_list.push_back(current_batch);
    }
  };

  for (int i = 0; i < polygon_count; i++) {
    auto& polygon = polygons[i];

    int vertices = (polygon.count - 2) * 3;

    RenderState current_state{};
    current_state.texture_params = &polygon.texture_params;
    current_state.alpha = polygon.params.alpha;
    current_state.enable_translucent_depth_write = polygon.params.enable_translucent_depth_write;

    // make sure current batch state is initialized
    if (i == 0) {
      current_batch.state = current_state;
    }

    if (current_state != current_batch.state) {
      FinalizeBatch();
      current_batch = {current_state, next_batch_start, 0};
    }

    next_batch_start += vertices;
  }

  FinalizeBatch();

  for (auto const& batch : batch_list) {
    int alpha = batch.state.alpha;
    bool wireframe = false;

    if (alpha == 0) {
      wireframe = true;
      alpha = 31;
    }

    if (translucent == (alpha != 31)) {
      glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

      auto texture_params = (GPU::TextureParams const*)batch.state.texture_params;

      auto format = texture_params->format;
      bool use_map = disp3dcnt.enable_textures && format != GPU::TextureParams::Format::None;
      bool use_alpha_test = disp3dcnt.enable_alpha_test;

      program->SetUniformBool("u_use_map", use_map);

      if(use_map) {
        GLuint texture = texture_cache.Get(texture_params);
        // @todo: bind texture the right way
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // update alpha-test uniforms
        program->SetUniformBool("u_use_alpha_test", use_alpha_test);
        program->SetUniformFloat("u_alpha_test_threshold", (float)alpha_test.alpha / 31.0f);
      }

      program->SetUniformFloat("u_polygon_alpha", (float)alpha / 31.0f);

      if (batch.state.enable_translucent_depth_write || (
        alpha == 31 && !texture_params->color0_transparent && (
          format == Format::Palette2BPP ||
          format == Format::Palette4BPP ||
          format == Format::Palette8BPP))) {
        glDepthMask(GL_TRUE);
        glDrawArrays(GL_TRIANGLES, batch.vertex_start, batch.vertex_count);
      } else {
        // @todo: make sure that the depth test does not break

        // render opaque pixels to the depth-buffer only first.
        program->SetUniformBool("u_discard_translucent_pixels", true);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_TRUE);
        glDrawArrays(GL_TRIANGLES, batch.vertex_start, batch.vertex_count);

        // then render both opaque and translucent pixels without updating the depth buffer.
        program->SetUniformBool("u_discard_translucent_pixels", false);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_FALSE);
        glDrawArrays(GL_TRIANGLES, batch.vertex_start, batch.vertex_count);
      }
    }
  }
}

void OpenGLRenderer::SetupAndUploadVBO(void const* polygons_, int polygon_count) {
  auto polygons = (GPU::Polygon const*)polygons_;

  vertex_buffer.clear();

  for (int i = 0; i < polygon_count; i++) {
    auto& polygon = polygons[i];

    for (int j = 1; j < polygon.count - 1; j++) {
      for (int k : {0, j, j + 1}) {
        auto vert = polygon.vertices[k];

        float position_x = (float)vert->position.x().raw() / (float)(1 << 12);
        float position_y = (float)vert->position.y().raw() / (float)(1 << 12);
        float position_z = (float)vert->position.z().raw() / (float)(1 << 12);
        float position_w = (float)vert->position.w().raw() / (float)(1 << 12);

        float color_a = (float)vert->color.a().raw() / (float)(1 << 6);
        float color_r = (float)vert->color.r().raw() / (float)(1 << 6);
        float color_g = (float)vert->color.g().raw() / (float)(1 << 6);
        float color_b = (float)vert->color.b().raw() / (float)(1 << 6);

        float texcoord_s = (float)vert->uv.x().raw() / (float)(1 << 4);
        float texcoord_t = (float)vert->uv.y().raw() / (float)(1 << 4);

        vertex_buffer.push_back({position_x, position_y, position_z, position_w, color_r, color_g, color_b, color_a, texcoord_s, texcoord_t});
      }
    }
  }

  vbo->Upload(vertex_buffer.data(), vertex_buffer.size());
}

bool OpenGLRenderer::RenderState::operator==(OpenGLRenderer::RenderState const& other) const {
  // @todo: move this back into the header file, once the GPU definition thing is fixed.
  return ((GPU::TextureParams*)texture_params)->raw_value == ((GPU::TextureParams*)other.texture_params)->raw_value &&
         alpha == other.alpha &&
         enable_translucent_depth_write == other.enable_translucent_depth_write;
}

} // namespace lunar::nds