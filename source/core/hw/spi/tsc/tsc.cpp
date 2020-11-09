/*
 * Copyright (C) fleroviux
 */

#include <common/log.hpp>

#include "tsc.hpp"

namespace fauxDS::core {

void TSC::Reset() {
  data_reg = 0;
}

void TSC::SetTouchState(bool pressed, int x, int y) {
  if (pressed) {
    // TODO: read this information from the firmware.bin file.
    u16 adc_x1 = 0;
    u16 adc_y1 = 0;
    u16 scr_x1 = 0;
    u16 scr_y1 = 0;
    u16 adc_x2 = 0xFF0;
    u16 adc_y2 = 0xBF0;
    u16 scr_x2 = 255;
    u16 scr_y2 = 191;

    position_x = (x - scr_x1 + 1) * (adc_x2 - adc_x1) / (scr_x2 - scr_x1) + adc_x1;
    position_y = (y - scr_y1 + 1) * (adc_y2 - adc_y1) / (scr_y2 - scr_y1) + adc_y1;

    LOG_INFO("debug x_in={0} x_out={1:X}", x, position_x);
    LOG_INFO("debug y_in={0} y_out={1:X}", y, position_y);
  } else {
    position_x = 0;
    position_y = 0;
  }
}

auto TSC::Transfer(u8 data) -> u8 {
  ASSERT(data == 0 || (data & 128) != 0, "SPI: TSC: unhandled attempt to start command mid-byte.");

  u8 out = (data_reg >> 5) & 0xFF;

  data_reg <<= 8;
  data_reg &= 0xFFF;

  if (data & 128) {
    auto channel = (data >> 4) & 7;

    ASSERT(data_reg == 0, "SPI: TSC: attempted to send next command before all data was read.");

    switch (channel) {
      /// Y position
      case 1:
        data_reg = position_y;
        break;

      /// X position
      case 5:
        data_reg = position_x;
        break;

      default:
        data_reg = 0xFFF;
        break;
    }
  }

  return out;
}

} // namespace fauxDS::core
