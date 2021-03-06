/*
 * Copyright (C) 2021 fleroviux
 */

#define MINIAUDIO_IMPLEMENTATION

#include <util/log.hpp>

#include "audio_device.hpp"

void MADataCallback(
  ma_device* device,
  void* output,
  const void* input,
  u32 frame_count
) {
  auto user_data = (MAAudioDevice::UserData*)device->pUserData;

  user_data->callback(user_data->argument, (s16*)output, frame_count * sizeof(s16) * 2);
}

bool MAAudioDevice::Open(
  void* userdata,
  Callback callback,
  uint frequency,
  uint block_size
) {
  ma_device_config device_config;

  opened = false;

  // TODO: miniaudio does not support playback with a constant block size,
  // therefore we do not support it either right now.
  device_config = ma_device_config_init(ma_device_type_playback);
  device_config.playback.format = ma_format_s16;
  device_config.playback.channels = 2;
  device_config.sampleRate = frequency;
  device_config.dataCallback = MADataCallback;
  device_config.pUserData = &user_data;

  this->frequency = frequency;
  this->block_size = block_size;
  user_data = {userdata, callback};

  if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
    LOG_ERROR("MAAudioDevice: failed to open device.");
    return false;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    LOG_ERROR("MAAudioDevice: failed to start device.");
    return false;
  }

  opened = true;
  return true;
}
  
void MAAudioDevice::Close() {
  ma_device_uninit(&device);
}

void MAAudioDevice::SetPaused(bool paused) {
  if (opened) {
    if (paused) {
      ma_device_stop(&device);  
    } else {
      ma_device_start(&device);
    }
  }
}
