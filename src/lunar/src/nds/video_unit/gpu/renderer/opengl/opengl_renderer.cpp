/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Include gpu.hpp for definitions... (need to find a better solution for this)
#include "nds/video_unit/gpu/gpu.hpp"

#include "shader/test.glsl.hpp"
#include "opengl_renderer.hpp"

#include <SDL.h>
extern SDL_Window* g_window;

namespace lunar::nds {

OpenGLRenderer::OpenGLRenderer(
  Region<4, 131072> const& vram_texture,
  Region<8> const& vram_palette
)   : texture_cache{vram_texture, vram_palette} {
  glEnable(GL_DEPTH_TEST);

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
  glClearColor(0.01, 0.01, 0.01, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::RenderPolygons(void const* polygons_, int polygon_count, bool translucent) {
  auto polygons = (GPU::Polygon const*)polygons_;

  program->Use();
  vao->Bind();
  glDepthFunc(GL_LEQUAL);

  // @todo: combine polygons into batches to reduce draw calls
  int offset = 0;
  for (int i = 0; i < polygon_count; i++) {
    auto& polygon = polygons[i];

    int alpha = polygon.params.alpha;
    bool wireframe = false;

    if (alpha == 0) {
      wireframe = true;
      alpha = 31;
    }

    int real_vertices = (polygon.count - 2) * 3;

    if (translucent == (alpha != 31)) {
      glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

      // @todo: properly handle case when no texture is bound.
      if(polygon.texture_params.format != GPU::TextureParams::Format::None) {
        GLuint texture = texture_cache.Get(&polygon.texture_params);
        // @todo: bind texture the right way
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
      }

      glDrawArrays(GL_TRIANGLES, offset, real_vertices);
    }

    offset += real_vertices;
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

} // namespace lunar::nds