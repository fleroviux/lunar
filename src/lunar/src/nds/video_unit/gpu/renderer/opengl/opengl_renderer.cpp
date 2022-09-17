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

  test_program = CompileProgram(test_vert, test_frag).second;

  // Create VAO and VBO
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, k_total_vertices * sizeof(BufferVertex), nullptr, GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // clip-space position
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float))); // vertex color
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
}

OpenGLRenderer::~OpenGLRenderer() {
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

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_buffer.size() * sizeof(BufferVertex), vertex_buffer.data());

  glViewport(0, 384, 512, 384);
  glClearColor(0.01, 0.01, 0.01, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(test_program);
  glBindVertexArray(vao);

  glDrawArrays(GL_TRIANGLES, 0, vertex_buffer.size());

  SDL_GL_SwapWindow(g_window);
}

// -------------------------------------------------

auto OpenGLRenderer::CompileShader(
  GLenum type,
  char const* source
) -> std::pair<bool, GLuint> {
  char const* source_array[] = { source };

  auto shader = glCreateShader(type);

  glShaderSource(shader, 1, source_array, nullptr);
  glCompileShader(shader);

  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if(compiled == GL_FALSE) {
    GLint max_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

    auto error_log = std::make_unique<GLchar[]>(max_length);
    glGetShaderInfoLog(shader, max_length, &max_length, error_log.get());
    Log<Error>("OGLVideoDevice: failed to compile shader:\n{0}", error_log.get());
    return std::make_pair(false, shader);
  }

  return std::make_pair(true, shader);
}

auto OpenGLRenderer::CompileProgram(
  char const* vertex_src,
  char const* fragment_src
) -> std::pair<bool, GLuint> {
  auto [vert_success, vert_id] = CompileShader(GL_VERTEX_SHADER, vertex_src);
  auto [frag_success, frag_id] = CompileShader(GL_FRAGMENT_SHADER, fragment_src);

  if (!vert_success || !frag_success) {
    return std::make_pair<bool, GLuint>(false, 0);
  } else {
    auto prog_id = glCreateProgram();

    glAttachShader(prog_id, vert_id);
    glAttachShader(prog_id, frag_id);
    glLinkProgram(prog_id);
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);

    return std::make_pair(true, prog_id);
  }
}

} // namespace lunar::nds