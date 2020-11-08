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
  enum class State {
    ReceiveCommand,
    Send_8bit,
    Send_12bit_hi,
    Send_12bit_lo
  } state;

  u16 data_reg;

  //u16 position_x = 0;
  //u16 position_y = 0;
};

} // namespace fauxDS::core
