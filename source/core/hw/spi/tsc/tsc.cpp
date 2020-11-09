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
  ASSERT(data == 0 || (data & 128) != 0, "SPI: TSC: unhandled attempt to start command mid-byte.");

  u8 out = (data_reg >> 4) & 0xFF;
  data_reg <<= 8;
  data_reg &= 0xFFF;

  if (data & 128) {
    auto channel = (data >> 4) & 7;

    ASSERT(data_reg == 0, "SPI: TSC: attempted to send next command before all data was read.");

    switch (channel) {
      /// Y position
      case 1:
        data_reg = 0x100;
        break;

        /// X position
      case 5:
        data_reg = 0xED0;
        break;

      default:
        data_reg = 0xFFF;
        break;
    }
  }

  return out;
}

} // namespace fauxDS::core
