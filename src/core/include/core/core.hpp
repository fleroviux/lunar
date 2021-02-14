/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <core/device/audio_device.hpp>
#include <core/device/video_device.hpp>
#include <util/integer.hpp>
#include <string_view>

namespace Duality::core {

struct Core {
  Core(std::string_view rom_path);
 ~Core();

 void SetAudioDevice(AudioDevice& device);
 void SetVideoDevice(VideoDevice& device);

 void Run(uint cycles);

private:
  struct CoreImpl* pimpl = nullptr;
};

} // namespace Duality::core
