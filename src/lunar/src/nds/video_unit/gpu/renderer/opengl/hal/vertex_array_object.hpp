/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <GL/glew.h>
#include <cstddef>
#include <optional>

#include "buffer_object.hpp"

namespace lunar::nds {

struct VertexArrayObject {
  struct Attribute {
    BufferObject* buffer;
    int components;
    GLenum type;
    GLboolean normalized = GL_FALSE;
    GLsizei stride = 0;
    GLsizei offset = 0;
  };

  VertexArrayObject() {
    // TODO: this is not nice for keeping VAOs in class memory.
    glGenVertexArrays(1, &vao);
  }

 ~VertexArrayObject() {
    glDeleteVertexArrays(1, &vao);
  }

  [[nodiscard]] auto Handle() -> GLuint { // NOLINT(readability-make-member-function-const)
    return vao;
  }

  void Bind() {
    glBindVertexArray(vao);
  }

  void SetAttribute(GLuint index, std::optional<Attribute> attribute) { // NOLINT(readability-make-member-function-const)
    glBindVertexArray(vao);

    if (attribute.has_value()) {
      auto const& attr = attribute.value();
      glBindBuffer(GL_ARRAY_BUFFER, attr.buffer->Handle());
      glVertexAttribPointer(
        index, attr.components, attr.type, attr.normalized, attr.stride, (void const*)(std::intptr_t)attr.offset);
      glEnableVertexAttribArray(index);
    } else {
      glDisableVertexAttribArray(index);
    }

    glBindVertexArray(0);
  }

private:
  GLuint vao = 0;
};

} // namespace lunar::nds