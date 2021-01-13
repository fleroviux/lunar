/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>

namespace Duality::core {

struct SPIDevice {
  virtual ~SPIDevice() = default;

  virtual void Select() = 0;
  virtual void Deselect() = 0;
  virtual auto Transfer(u8 data) -> u8 = 0;
};

} // namespace Duality::core
