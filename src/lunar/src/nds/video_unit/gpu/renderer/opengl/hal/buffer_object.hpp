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

namespace lunar::nds {

struct BufferObject {
  BufferObject(GLenum target, size_t size, GLenum usage)
      : target(target), size(size) {
    // TODO: this is not nice for keeping *BOs in class memory.
    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, (GLsizeiptr)size, nullptr, usage);
  }

 ~BufferObject() {
    glDeleteBuffers(1, &buffer);
  }

  [[nodiscard]] auto Handle() -> GLuint { // NOLINT(readability-make-member-function-const)
    return buffer;
  }

  [[nodiscard]] auto Size() const -> size_t {
    return size;
  }

  template<typename T>
  void Upload(T const* data, size_t count, size_t offset = 0) {
    glBindBuffer(target, buffer);
    glBufferSubData(target, offset * sizeof(T), count * sizeof(T), data);
  }

private:
  GLenum target;
  size_t size;
  GLuint buffer = 0;
};

struct ArrayBufferObject : BufferObject {
  ArrayBufferObject(size_t size, GLenum usage) : BufferObject(GL_ARRAY_BUFFER, size, usage) {}
};

} // namespace lunar::nds