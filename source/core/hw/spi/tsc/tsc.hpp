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

  void Select() override;
  void Deselect() override;
  auto Transfer(u8 data) -> u8 override;

private:
  u16 data_reg;
};

} // namespace fauxDS::core
