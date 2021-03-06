/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <core/device/audio_device.hpp>

extern "C" {
#include <miniaudio.h>
}

struct MAAudioDevice final : Duality::core::AudioDevice {
  auto GetSampleRate() -> uint override { return frequency; }
  auto GetBlockSize() -> uint override { return block_size; }

  bool Open(
    void* userdata,
    Callback callback,
    uint frequency,
    uint block_size
  ) override;
  
  void Close() override;

  struct UserData {
    void* argument;
    Callback callback;
  };

private:
  uint frequency;
  uint block_size;
  UserData user_data;
  ma_device device;
};