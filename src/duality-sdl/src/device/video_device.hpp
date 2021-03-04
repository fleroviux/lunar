/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <core/device/video_device.hpp>
#include <GL/glew.h>
#include <SDL.h>

struct SDL2VideoDevice final : Duality::core::VideoDevice {
  SDL2VideoDevice(SDL_Window* window);
 ~SDL2VideoDevice() override;

  void Draw(u32 const* top, u32 const* bottom) override;
  void Present();

private:
  SDL_Window* window;
  u32 const* buffer_top = nullptr;
  u32 const* buffer_bottom = nullptr;
  GLuint textures[2];
};