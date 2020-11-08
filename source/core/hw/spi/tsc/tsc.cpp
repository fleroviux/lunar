/*
 * Copyright (C) fleroviux
 */

#include <common/log.hpp>

#include "tsc.hpp"

namespace fauxDS::core {

void TSC::Reset() {
}

void TSC::Select() {
}

void TSC::Deselect() {
}

auto TSC::Transfer(u8 data) -> u8 {
  LOG_INFO("SPI: TSC: received byte 0x{0:02X}", data);
  ASSERT(data == 0 || (data & 128) != 0, "SPI: TSC: expected command but start bit is not set!");

  //bool mode_8bit = (data >> 3) & 1;
  auto channel = (data >> 4) & 7;

  u8 out = (data_reg >> 4) & 0xFF;
  data_reg <<= 8;
  data_reg &= 0xFFF;
  if (!(data & 128)) {
    return out;
  }

  switch (channel) {
    /// Temperature 0
    case 0:
      data_reg = 0;
      break;

    /// Y position
    case 1:
      data_reg = 0x100;
      break;

    /// Z1 position
    case 3:
      data_reg = 0x20;
      break;
    /// Z2 position
    case 4:
      data_reg = 0xFF0;
      break;

    /// X position
    case 5:
      data_reg = 0xED0;
      break;

    default: {
      ASSERT(false, "SPI: TSC: unhandled channel {0}", channel);
    }
  }

  return out;
}

} // namespace fauxDS::core
