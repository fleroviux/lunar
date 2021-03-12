/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <core/device/input_device.hpp>

#include "hw/spi/spi_device.hpp"
#include "hw/spi/firmware/firmware.hpp"

namespace Duality::Core {

/// Touch Screen Controller
/// Asahi Kasei Microsystems AK4148AVT
struct TSC : SPIDevice {
  void Reset(Firmware& firmware);
  void SetInputDevice(InputDevice& device);

  void Select() override {}
  void Deselect() override {}
  auto Transfer(u8 data) -> u8 override;

private:
  u16 data_reg;

  /// Firmware calibration data
  u16 adc_x1;
  u16 adc_y1;
  u16 adc_x2;
  u16 adc_y2;
  u8  scr_x2;
  u8  scr_y2;
  u8  scr_x1;
  u8  scr_y1;

  InputDevice* input_device = nullptr;
};

} // namespace Duality::Core
