/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <GL/glew.h>

namespace lunar {

struct Texture2D {
  static auto Create(
    int width,
    int height,
    GLint internal_format,
    GLenum format,
    GLenum type,
    void const* data = nullptr
  ) -> Texture2D* {
    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return new Texture2D{width, height, internal_format, format, type, texture};
  }

 ~Texture2D() {
    glDeleteTextures(1, &texture);
  }

  [[nodiscard]] auto Handle() -> GLuint { // NOLINT(readability-make-member-function-const)
    return texture;
  }

  void SetMinFilter(GLint filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  }

  void SetMagFilter(GLint filter) {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  }

  void SetWrapBehaviour(GLint wrap_s, GLint wrap_t) {
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
  }

  void Bind() { // NOLINT(readability-make-member-function-const)
    glBindTexture(GL_TEXTURE_2D, texture);
  }

  void Bind(GLenum texture_slot) { // NOLINT(readability-make-member-function-const)
    glActiveTexture(texture_slot);
    glBindTexture(GL_TEXTURE_2D, texture);
  }

  void Upload(void const* data) {
    Bind();
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);
  }

private:
  Texture2D(
    int width,
    int height,
    GLint internal_format,
    GLenum format,
    GLenum type,
    GLuint texture
  ) : width(width), height(height), internal_format(internal_format), format(format), type(type), texture(texture) {}

  int width;
  int height;
  GLint internal_format;
  GLenum format;
  GLenum type;
  GLuint texture;
};

} // namespace lunar