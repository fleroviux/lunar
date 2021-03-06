/*
 * Copyright (C) 2021 fleroviux
 */

#define MINIAUDIO_IMPLEMENTATION

#include <util/log.hpp>

#include "audio_device.hpp"

bool MAAudioDevice::Open(
  void* userdata,
  Callback callback,
  uint frequency,
  uint block_size
) {
  ma_device_config device_config;

  // TODO: miniaudio does not support playback with a constant block size,
  // therefore we do not support it either right now.
  device_config = ma_device_config_init(ma_device_type_playback);
  device_config.playback.format = ma_format_s16;
  device_config.playback.channels = 2;
  device_config.sampleRate = frequency;
  device_config.dataCallback = MADataCallback;
  device_config.pUserData = this;

  this->userdata = userdata;
  this->callback = callback;
  this->frequency = frequency;
  this->block_size = block_size;

  if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
    LOG_ERROR("MAAudioDevice: failed to open device.");
    return false;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    LOG_ERROR("MAAudioDevice: failed to start device.");
    return false;
  }

  return true;
}
  
void MAAudioDevice::Close() {
  ma_device_uninit(&device);
}

void MADataCallback(
  ma_device* device,
  void* output,
  const void* input,
  u32 frame_count
) {
  auto audio_device = (MAAudioDevice*)device->pUserData;

  audio_device->callback(audio_device->userdata, (s16*)output, frame_count * sizeof(s16) * 2);
}