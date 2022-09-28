/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunar/device/video_device.hpp>
#include <GL/glew.h>
#include <SDL.h>

namespace lunar {

struct OGLVideoDevice final : VideoDevice {
  explicit OGLVideoDevice(SDL_Window* window);

 ~OGLVideoDevice() override;

  void Draw(
    ImageType top_image_type,
    void const* top_image,
    ImageType bottom_image_type,
    void const* bottom_image
  ) override;

  void Present();

private:
  SDL_Window* window;
  void const* top_image = nullptr;
  void const* bottom_image = nullptr;
  ImageType top_image_type = ImageType::Software;
  ImageType bottom_image_type = ImageType::Software;
  GLuint textures[2];
};

} // namespace lunar