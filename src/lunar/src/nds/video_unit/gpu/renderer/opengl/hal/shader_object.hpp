/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunar/log.hpp>
#include <GL/glew.h>

namespace lunar::nds {

struct ShaderObject {
  static auto CreateVert(char const* source) -> ShaderObject* {
    return Create(GL_VERTEX_SHADER, source);
  }

  static auto CreateFrag(char const* source) -> ShaderObject* {
    return Create(GL_FRAGMENT_SHADER, source);
  }

 ~ShaderObject() {
    glDeleteShader(shader);
  }

  [[nodiscard]] auto Handle() -> GLuint { // NOLINT(readability-make-member-function-const)
    return shader;
  }

private:
  explicit ShaderObject(GLuint shader) : shader(shader) {}

  static auto Create(GLenum type, char const* source) -> ShaderObject* {
    char const* source_array[] = { source };

    GLint compiled = 0;
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, source_array, nullptr);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (compiled == GL_FALSE) {
      GLint max_length = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

      auto error_log = new GLchar[max_length];
      glGetShaderInfoLog(shader, max_length, &max_length, error_log);
      Assert(false, "Failed to compile shader:\n{0}", error_log);

      delete[] error_log;
      return nullptr;
    }

    return new ShaderObject{shader};
  }

  GLuint shader;
};

} // namespace lunar::nds