/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <platform/device/ogl_video_device.hpp>

extern GLuint opengl_final_texture;

namespace lunar {

OGLVideoDevice::OGLVideoDevice(SDL_Window* window) : window(window) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);

  glGenTextures(2, &textures[0]);

  for (int i = 0; i < 2; i++) {
    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  glClearColor(0, 0, 0, 1);
}

OGLVideoDevice::~OGLVideoDevice() {
  glDeleteTextures(2, &textures[0]);
}

void OGLVideoDevice::Draw(
  ImageType top_image_type,
  void const* top_image,
  ImageType bottom_image_type,
  void const* bottom_image
) {
  this->top_image_type = top_image_type;
  this->bottom_image_type = bottom_image_type;
  this->top_image = top_image;
  this->bottom_image = bottom_image;
}

void OGLVideoDevice::Present() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glViewport(0, 0, 512, 768);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);

  if(top_image_type == ImageType::Software) {
    glBindTexture(GL_TEXTURE_2D, textures[0]);

    if(top_image != nullptr) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_BGRA, GL_UNSIGNED_BYTE, top_image);
    }
  } else {
    glBindTexture(GL_TEXTURE_2D, (GLuint)(std::uintptr_t)top_image);
  }

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(-1.0f,  1.0f);
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f( 1.0f,  1.0f);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f( 1.0f,  0.0f);
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(-1.0f,  0.0f);
  glEnd();

  if(bottom_image_type == ImageType::Software) {
    glBindTexture(GL_TEXTURE_2D, textures[1]);

    if(bottom_image != nullptr) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_BGRA, GL_UNSIGNED_BYTE, bottom_image);
    }
  } else {
    glBindTexture(GL_TEXTURE_2D, (GLuint)(std::uintptr_t)bottom_image);
  }

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(-1.0f,  0.0f);
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f( 1.0f,  0.0f);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f( 1.0f, -1.0f);
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(-1.0f, -1.0f);
  glEnd();

  SDL_GL_SwapWindow(window);
}

} // namespace lunar