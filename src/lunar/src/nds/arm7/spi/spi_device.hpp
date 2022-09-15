
// Copyright (C) 2022 fleroviux

#pragma once

#include <lunar/integer.hpp>

namespace lunar::nds {

struct SPIDevice {
  virtual ~SPIDevice() = default;

  virtual void Reset() = 0;
  virtual void Select() = 0;
  virtual void Deselect() = 0;
  virtual auto Transfer(u8 data) -> u8 = 0;
};

} // namespace lunar::nds
