/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <GL/glew.h>

#include "shader_object.hpp"

namespace lunar::nds {

struct ProgramObject {
  static auto Create(char const* vert, char const* frag) -> ProgramObject* {
    auto vert_shader = ShaderObject::CreateVert(vert);
    auto frag_shader = ShaderObject::CreateFrag(frag);

    if (vert_shader == nullptr || frag_shader == nullptr) {
      delete vert_shader;
      delete frag_shader;
      return nullptr;
    }

    return Create(vert_shader, frag_shader, true);
  }

  static auto Create(
    ShaderObject* vert,
    ShaderObject* frag,
    bool auto_delete = false
  ) -> ProgramObject* {
    GLuint program = glCreateProgram();

    glAttachShader(program, vert->Handle());
    glAttachShader(program, frag->Handle());
    glLinkProgram(program);

    if (auto_delete) {
      delete vert;
      delete frag;
    }

    return new ProgramObject{program};
  }

 ~ProgramObject() {
    glDeleteProgram(program);
  }

  [[nodiscard]] auto Handle() -> GLuint {
    return program;
  }

  void Use() {
    glUseProgram(program);
  }

private:
  explicit ProgramObject(GLuint program) : program(program) {}

  GLuint program;
};

} // namespace lunar::nds