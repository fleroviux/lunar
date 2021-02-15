/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <core/device/audio_device.hpp>
#include <core/device/input_device.hpp>
#include <core/device/video_device.hpp>
#include <util/integer.hpp>
#include <string>

namespace Duality::core {

struct Core {
  Core(std::string const& rom_path);
 ~Core();

 void SetAudioDevice(AudioDevice& device);
 void SetInputDevice(InputDevice& device);
 void SetVideoDevice(VideoDevice& device);

 void Run(uint cycles);

private:
  struct CoreImpl* pimpl = nullptr;
};

} // namespace Duality::core
