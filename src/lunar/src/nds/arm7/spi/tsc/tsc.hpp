/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunar/device/input_device.hpp>

#include "nds/arm7/spi/spi_device.hpp"
#include "nds/cart/backup/flash.hpp"

namespace lunar::nds {

/* Touch Screen Controller
 * Asahi Kasei Microsystems AK4148AVT
 */
struct TSC : SPIDevice {
  TSC(FLASH& firmware);

  void Reset() override;
  void Select() override {}
  void Deselect() override {}
  auto Transfer(u8 data) -> u8 override;

  void SetInputDevice(InputDevice& device);

private:
  u16 data_reg;

  // Firmware calibration data
  u16 adc_x1;
  u16 adc_y1;
  u16 adc_x2;
  u16 adc_y2;
  u8  scr_x2;
  u8  scr_y2;
  u8  scr_x1;
  u8  scr_y1;

  FLASH& firmware;
  InputDevice* input_device = nullptr;
};

} // namespace lunar::nds
