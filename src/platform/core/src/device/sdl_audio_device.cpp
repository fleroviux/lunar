/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunar/log.hpp>
#include <platform/device/sdl_audio_device.hpp>

namespace lunar {

bool SDL2AudioDevice::Open(
  void* userdata,
  Callback callback,
  uint frequency,
  uint samples
) {
  auto want = SDL_AudioSpec{};

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    LOG_ERROR("SDL_Init(SDL_INIT_AUDIO) failed.");
    return false;
  }

  want.freq = frequency;
  want.samples = samples;
  want.format = AUDIO_S16;
  want.channels = 2;
  want.callback = (SDL_AudioCallback)callback;
  want.userdata = userdata;

  device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

  if (device == 0) {
    LOG_ERROR("SDL_OpenAudioDevice: failed to open audio: %s\n", SDL_GetError());
    return false;
  }

  if (have.format != want.format) {
    LOG_ERROR("SDL_AudioDevice: S16 sample format unavailable.");
    return false;
  }

  if (have.channels != want.channels) {
    LOG_ERROR("SDL_AudioDevice: Stereo output unavailable.");
    return false;
  }

  SDL_PauseAudioDevice(device, 0);
  return true;
}

void SDL2AudioDevice::Close() {
  SDL_CloseAudioDevice(device);
}

} // namespace lunar