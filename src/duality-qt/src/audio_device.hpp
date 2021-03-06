/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <core/device/audio_device.hpp>

extern "C" {
#include <miniaudio.h>
}

void MADataCallback(
  ma_device* device,
  void* output,
  const void* input,
  u32 frame_count
);

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

private:
  friend void MADataCallback(
    ma_device* device,
    void* output,
    const void* input,
    u32 frame_count);

  void* userdata;
  Callback callback;
  uint frequency;
  uint block_size;

  ma_device device;
};