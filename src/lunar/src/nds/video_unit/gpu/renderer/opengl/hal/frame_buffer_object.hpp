/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"
#pragma ide diagnostic ignored "readability-make-member-function-const"

#pragma once

#include <GL/glew.h>
#include <initializer_list>

#include "texture_2d.hpp"

namespace lunar::nds {

struct FrameBufferObject {
  static auto Create() -> FrameBufferObject* {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    return new FrameBufferObject{fbo};
  }

 ~FrameBufferObject() {
    glDeleteFramebuffers(1, &fbo);
  }

  void Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  }

  void Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void Attach(GLenum attachment, Texture2D* texture) {
    Bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->Handle(), 0);
  }

  void Detach(GLenum attachment) {
    Bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, 0, 0);
  }

  void DrawBuffers(std::initializer_list<GLenum> attachments) {
    Bind();
    glDrawBuffers((GLsizei)attachments.size(), attachments.begin());
  }

private:
  explicit FrameBufferObject(GLuint fbo) : fbo{fbo} {}

  GLuint fbo;
};

} // namespace lunar::nds

#pragma clang diagnostic pop