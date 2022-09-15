
// Copyright (C) 2022 fleroviux

#pragma once

#include <lunar/integer.hpp>

namespace lunar {

struct VideoDevice {
  virtual ~VideoDevice() = default;

  virtual void Draw(u32 const* top, u32 const* bottom) = 0;
};

} // namespace lunar::nds
