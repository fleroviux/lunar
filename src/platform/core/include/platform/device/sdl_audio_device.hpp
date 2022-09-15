
// Copyright (C) 2022 fleroviux

#pragma once

#include <lunar/device/audio_device.hpp>
#include <SDL.h>

namespace lunar {

struct SDL2AudioDevice final : AudioDevice {
  auto GetSampleRate() -> uint override { return have.freq; }
  auto GetBlockSize() -> uint override { return have.samples; }

  bool Open(
    void* userdata,
    Callback callback,
    uint frequency,
    uint samples
  ) override;
  
  void Close() override;

private:
  SDL_AudioDeviceID device;
  SDL_AudioSpec have;
};

} // namespace lunar