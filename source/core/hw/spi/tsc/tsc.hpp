/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <core/hw/spi/spi_device.hpp>
#include <core/hw/spi/firmware/firmware.hpp>

namespace Duality::core {

/// Touch Screen Controller
/// Asahi Kasei Microsystems AK4148AVT
struct TSC : SPIDevice {
  void Reset(Firmware& firmware);

  void SetTouchState(bool pressed, int x, int y);

  void Select() override {}
  void Deselect() override {}
  auto Transfer(u8 data) -> u8 override;

private:
  u16 data_reg;
  u16 position_x;
  u16 position_y;

  /// Firmware calibration data
  u16 adc_x1;
  u16 adc_y1;
  u16 adc_x2;
  u16 adc_y2;
  u8  scr_x2;
  u8  scr_y2;
  u8  scr_x1;
  u8  scr_y1;
};

} // namespace Duality::core
