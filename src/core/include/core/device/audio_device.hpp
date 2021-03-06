/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/integer.hpp>

namespace Duality::core {
  
struct AudioDevice {
  virtual ~AudioDevice() = default;

  using Callback = void (*)(void* userdata, s16* stream, int byte_len);

  virtual auto GetSampleRate() -> uint = 0;
  virtual auto GetBlockSize() -> uint = 0;

  virtual bool Open(
    void* userdata,
    Callback callback,
    uint frequency,
    uint samples
  ) = 0;

  virtual void Close() = 0;
};

} // namespace Duality::core
