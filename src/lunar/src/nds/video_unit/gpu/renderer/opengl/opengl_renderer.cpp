/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Include gpu.hpp for definitions... (need to find a better solution for this)
#include "nds/video_unit/gpu/gpu.hpp"

#include "opengl_renderer.hpp"

#include <SDL.h>
extern SDL_Window* g_window;

namespace lunar::nds {

OpenGLRenderer::OpenGLRenderer() {
}

OpenGLRenderer::~OpenGLRenderer() {
}

void OpenGLRenderer::Render(void const* polygons_, int polygon_count) {
  auto polygons = (GPU::Polygon const*)polygons_;

  // TODO: do not do this every frame...
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glViewport(0, 384, 512, 384);
  glClearColor(0.01, 0.01, 0.01, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  for (int i = 0; i < polygon_count; i++) {
    auto& polygon = polygons[i];

    glBegin(GL_TRIANGLE_FAN);

    for(int j = 0; j < polygon.count; j++) {
      auto vert = polygon.vertices[j];

      float position_x = (float)vert->position.x().raw() / (float) (1 << 12);
      float position_y = (float)vert->position.y().raw() / (float) (1 << 12);
      float position_z = (float)vert->position.z().raw() / (float) (1 << 12);
      float position_w = (float)vert->position.w().raw() / (float) (1 << 12);

      float color_a = (float) vert->color.a().raw() / (float) (1 << 6);
      float color_r = (float) vert->color.r().raw() / (float) (1 << 6);
      float color_g = (float) vert->color.g().raw() / (float) (1 << 6);
      float color_b = (float) vert->color.b().raw() / (float) (1 << 6);

      // TODO: handle w-buffering mode
      glColor4f(color_r, color_g, color_b, color_a);
      glVertex4f(position_x, position_y, position_z, position_w);
    }

    glEnd();
  }

  SDL_GL_SwapWindow(g_window);
}

} // namespace lunar::nds