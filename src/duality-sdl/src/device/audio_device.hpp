/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/log.hpp>
#include <core/device/audio_device.hpp>
#include <SDL.h>

struct SDL2AudioDevice final : Duality::core::AudioDevice {
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
