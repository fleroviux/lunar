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

  void SetImageTypeTop(ImageType type) override;
  void SetImageTypeBottom(ImageType type) override;
  void Draw(void const* top, void const* bottom) override;
  void Present();

private:
  SDL_Window* window;
  void const* buffer_top = nullptr;
  void const* buffer_bottom = nullptr;
  ImageType image_type_top = ImageType::Software;
  ImageType image_type_bottom = ImageType::Software;
  GLuint textures[2];
};

} // namespace lunar