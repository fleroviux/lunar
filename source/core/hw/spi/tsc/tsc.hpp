/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <core/hw/spi/spi_device.hpp>

namespace fauxDS::core {

/// Touch Screen Controller
/// Asahi Kasei Microsystems AK4148AVT
struct TSC : SPIDevice {
  void Reset();

  void SetTouchState(bool pressed, int x, int y);

  void Select() override {}
  void Deselect() override {}
  auto Transfer(u8 data) -> u8 override;

private:
  u16 data_reg;
  u16 position_x;
  u16 position_y;
};

} // namespace fauxDS::core
