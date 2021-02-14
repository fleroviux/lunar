/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/integer.hpp>

namespace Duality::core {

struct VideoDevice {
  virtual ~VideoDevice() = default;

  virtual void Draw(uint screen, u32* data) = 0;
};

} // namespace Duality::core
