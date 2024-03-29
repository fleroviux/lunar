/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/integer.hpp>

namespace lunar {
  
class AudioDevice {
  public:
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
