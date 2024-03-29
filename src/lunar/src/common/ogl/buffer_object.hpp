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

namespace lunar {

class BufferObject {
  public:
    static auto CreateArrayBuffer(size_t size, GLenum usage) -> BufferObject* {
      return new BufferObject{GL_ARRAY_BUFFER, size, usage};
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
    BufferObject(GLenum target, size_t size, GLenum usage)
        : target(target), size(size) {
      glGenBuffers(1, &buffer);
      glBindBuffer(target, buffer);
      glBufferData(target, (GLsizeiptr)size, nullptr, usage);
    }

    GLenum target;
    size_t size;
    GLuint buffer = 0;
};

} // namespace lunar