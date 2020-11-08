/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/integer.hpp>

namespace fauxDS::core {

struct SPIDevice {
  virtual ~SPIDevice() = default;

  virtual void Select() = 0;
  virtual void Deselect() = 0;
  virtual auto Transfer(u8 data) -> u8 = 0;
};

} // namespace fauxDS::core
