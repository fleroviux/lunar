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
  OGLVideoDevice(SDL_Window* window);
 ~OGLVideoDevice() override;

  void Draw(u32 const* top, u32 const* bottom) override;
  void Present();

private:
  SDL_Window* window;
  u32 const* buffer_top = nullptr;
  u32 const* buffer_bottom = nullptr;
  GLuint textures[2];
};

} // namespace lunar