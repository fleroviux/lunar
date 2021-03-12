/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/integer.hpp>

namespace Duality::Core {

struct VideoDevice {
  virtual ~VideoDevice() = default;

  virtual void Draw(u32 const* top, u32 const* bottom) = 0;
};

} // namespace Duality::Core
