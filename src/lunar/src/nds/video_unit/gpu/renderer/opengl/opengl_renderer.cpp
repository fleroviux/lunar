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

OpenGLRenderer::OpenGLRenderer() {
  glEnable(GL_DEPTH_TEST);

  program = ProgramObject::Create(test_vert, test_frag);

  vbo = BufferObject::CreateArrayBuffer(sizeof(BufferVertex) * k_total_vertices, GL_DYNAMIC_DRAW);
  vao = VertexArrayObject::Create();
  vao->SetAttribute(0, VertexArrayObject::Attribute{vbo, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0});
  vao->SetAttribute(1, VertexArrayObject::Attribute{vbo, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 4 * sizeof(float)});
}

OpenGLRenderer::~OpenGLRenderer() {
  delete program;
  delete vao;
  delete vbo;
}

void OpenGLRenderer::Render(void const* polygons_, int polygon_count) {
  auto polygons = (GPU::Polygon const*)polygons_;

  // TODO: do not do this every frame...
  glDepthFunc(GL_LEQUAL);

  vertex_buffer.clear();

  for (int i = 0; i < polygon_count; i++) {
    auto& polygon = polygons[i];

    for (int j = 1; j < polygon.count - 1; j++) {
      for (int k : {0, j, j + 1}) {
        auto vert = polygon.vertices[k];

        float position_x = (float) vert->position.x().raw() / (float) (1 << 12);
        float position_y = (float) vert->position.y().raw() / (float) (1 << 12);
        float position_z = (float) vert->position.z().raw() / (float) (1 << 12);
        float position_w = (float) vert->position.w().raw() / (float) (1 << 12);

        float color_a = (float) vert->color.a().raw() / (float) (1 << 6);
        float color_r = (float) vert->color.r().raw() / (float) (1 << 6);
        float color_g = (float) vert->color.g().raw() / (float) (1 << 6);
        float color_b = (float) vert->color.b().raw() / (float) (1 << 6);

        vertex_buffer.push_back({position_x, position_y, position_z, position_w, color_r, color_g, color_b, color_a});
      }
    }
  }

  vbo->Upload(vertex_buffer.data(), vertex_buffer.size());

  glViewport(0, 384, 512, 384);
  glClearColor(0.01, 0.01, 0.01, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  program->Use();
  vao->Bind();
  glDrawArrays(GL_TRIANGLES, 0, vertex_buffer.size());

  SDL_GL_SwapWindow(g_window);
}

} // namespace lunar::nds