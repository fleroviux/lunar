/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <util/integer.hpp>

namespace Duality::Core {

struct SPIDevice {
  virtual ~SPIDevice() = default;

  virtual void Reset() = 0;
  virtual void Select() = 0;
  virtual void Deselect() = 0;
  virtual auto Transfer(u8 data) -> u8 = 0;
};

} // namespace Duality::Core
