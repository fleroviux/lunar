
// Copyright (C) 2022 fleroviux

#pragma once

#include <lunar/integer.hpp>

namespace lunar {
  
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

} // namespace lunar
