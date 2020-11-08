/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "spi.hpp"

namespace fauxDS::core {

SPI::SPI() {
  Reset();
}

void SPI::Reset() {
  // TODO: reset SPICNT and SPIDATA registers
  //spicnt = { *this );
  //spidata = { *this }
}

auto SPI::SPICNT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return baudrate | (busy ? 128 : 0);
    case 1:
      return static_cast<u8>(device) |
            (bugged_hword_mode ?   4 : 0) |
            (chipselect_hold   ?   8 : 0) |
            (enable_irq        ?  64 : 0) |
            (enable            ? 128 : 0);
  }

  UNREACHABLE;
}

void SPI::SPICNT::WriteByte(uint offset, u8 value) {
  // TODO: verify that baudrate matches the selected device.
  switch (offset) {
    case 0:
      baudrate = value & 3;
      break;
    case 1:
      device = static_cast<Device>(value & 3);
      bugged_hword_mode = value & 4;
      chipselect_hold = value & 8;
      enable_irq = value & 64;
      enable = value & 128;
      break;
    default:
      UNREACHABLE;
  }
}

auto SPI::SPIDATA::ReadByte() -> u8 {
  return value;
}

void SPI::SPIDATA::WriteByte(u8 value) {
  if (spi.spicnt.enable && spi.spicnt.device == Device::Touchscreen)
    LOG_ERROR("accessing touchscreen!!!");
}

} // namespace fauxDS::core
