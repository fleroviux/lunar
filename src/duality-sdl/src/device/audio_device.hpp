/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/log.hpp>
#include <core/device/audio_device.hpp>
#include <SDL.h>

struct SDL2AudioDevice : public Duality::core::AudioDevice {
  auto GetSampleRate() -> int final { return have.freq; }
  auto GetBlockSize() -> int final { return have.samples; }

  bool Open(
    void* userdata,
    Callback callback,
    uint frequency,
    uint samples
  ) final;
  
  void Close() final;

private:
  SDL_AudioDeviceID device;
  SDL_AudioSpec have;
};
