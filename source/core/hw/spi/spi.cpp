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
      return 0;
    case 1:
      return 0;
  }

  UNREACHABLE;
}

void SPI::SPICNT::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      break;
    case 1:
      break;
    default:
      UNREACHABLE;
  }
}

auto SPI::SPIDATA::ReadByte() -> u8 {
  return value;
}

void SPI::SPIDATA::WriteByte(u8 value) {
}

} // namespace fauxDS::core
