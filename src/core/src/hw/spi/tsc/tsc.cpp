/*
 * Copyright (C) fleroviux
 */

#include <util/log.hpp>

#include "tsc.hpp"

namespace Duality::Core {

void TSC::Reset(Firmware& firmware) {
  data_reg = 0;

  // Read touchscreen calibration data from firmware.
  u32 userdata_offset;

  // cmd = READ, address = 0x000020
  firmware.Select();
  firmware.Transfer(0x03);
  firmware.Transfer(0x00);
  firmware.Transfer(0x00);
  firmware.Transfer(0x20);
  userdata_offset  = firmware.Transfer(0) << 0;
  userdata_offset |= firmware.Transfer(0) << 8;
  firmware.Deselect();

  // get TSC calibration data address from userdata offset.
  userdata_offset *= 8;
  userdata_offset += 0x58;

  // cmd = READ, address = TSC calibration data offset
  firmware.Select();
  firmware.Transfer(0x03);
  firmware.Transfer((userdata_offset >> 16) & 0xFF);
  firmware.Transfer((userdata_offset >>  8) & 0xFF);
  firmware.Transfer((userdata_offset >>  0) & 0xFF);
  adc_x1  = firmware.Transfer(0) << 0;
  adc_x1 |= firmware.Transfer(0) << 8;
  adc_y1  = firmware.Transfer(0) << 0;
  adc_y1 |= firmware.Transfer(0) << 8;
  scr_x1  = firmware.Transfer(0);
  scr_y1  = firmware.Transfer(0);
  adc_x2  = firmware.Transfer(0) << 0;
  adc_x2 |= firmware.Transfer(0) << 8;
  adc_y2  = firmware.Transfer(0) << 0;
  adc_y2 |= firmware.Transfer(0) << 8;
  scr_x2  = firmware.Transfer(0);
  scr_y2  = firmware.Transfer(0);
  firmware.Deselect();
}

void TSC::SetInputDevice(InputDevice& device) {
  input_device = &device;
}

auto TSC::Transfer(u8 data) -> u8 {
  u8 out = data_reg >> 8;
  data_reg <<= 8;

  if (data & 128) {
    auto channel = (data >> 4) & 7;
    
    u16 position_x = 0;
    u16 position_y = 0xFFF;

    if (input_device != nullptr && input_device->IsKeyDown(InputDevice::Key::TouchPen)) {
      auto point = input_device->GetTouchPoint();
      position_x = (point.x - scr_x1 + 1) * (adc_x2 - adc_x1) / (scr_x2 - scr_x1) + adc_x1;
      position_y = (point.y - scr_y1 + 1) * (adc_y2 - adc_y1) / (scr_y2 - scr_y1) + adc_y1;
    }

    switch (channel) {
      /// Y position
      case 1:
        data_reg = position_y << 3;
        break;

      /// X position
      case 5:
        data_reg = position_x << 3;
        break;

      default:
        data_reg = 0x7FF8;
        break;
    }
  }

  return out;
}

} // namespace Duality::Core
