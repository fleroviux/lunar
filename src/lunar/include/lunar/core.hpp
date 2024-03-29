/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/integer.hpp>
#include <lunar/device/audio_device.hpp>
#include <lunar/device/input_device.hpp>
#include <lunar/device/video_device.hpp>
#include <memory>
#include <string>

namespace lunar {

class CoreBase {
  public:
    virtual ~CoreBase() = default;

    virtual void Reset() = 0;

    virtual void SetAudioDevice(AudioDevice& device) = 0;
    virtual void SetInputDevice(InputDevice& device) = 0;
    virtual void SetVideoDevice(VideoDevice& device) = 0;

    virtual void Run(uint cycles) = 0;

    virtual void Load(std::string const& rom_path) = 0;
};

auto CreateCore() -> std::unique_ptr<CoreBase>;

} // namespace lunar
