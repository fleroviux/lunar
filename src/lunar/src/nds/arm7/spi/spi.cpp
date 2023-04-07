/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <atom/logger/logger.hpp>
#include <atom/panic.hpp>

#include "spi.hpp"

namespace lunar::nds {

SPI::SPI(IRQ& irq7)
    : irq7(irq7)
    , firmware("firmware.bin", FLASH::Size::_256K)
    , tsc(firmware) {
  Reset();
}

void SPI::Reset() {
  // TODO: reset SPICNT and SPIDATA registers
  //spicnt = { *this );
  //spidata = { *this }
  firmware.Reset();
  tsc.Reset();
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

  ATOM_UNREACHABLE();
}

void SPI::SPICNT::WriteByte(uint offset, u8 value) {
  int device_old = device;
  bool enable_old = enable;

  // TODO: verify that baudrate matches the selected device.
  switch (offset) {
    case 0:
      baudrate = value & 3;
      break;
    case 1:
      device = value & 3;
      bugged_hword_mode = value & 4;
      chipselect_hold = value & 8;
      enable_irq = value & 64;
      enable = value & 128;
      if ((enable_old && !enable) || (enable && device != device_old)) {
        if (spi.devices[device_old] != nullptr)
          spi.devices[device_old]->Deselect();
      }
      if ((!enable_old && enable) || (enable && device != device_old)) {
        if (spi.devices[device] != nullptr)
          spi.devices[device]->Select();
      }
      break;
    default:
      ATOM_UNREACHABLE();
  }
}

auto SPI::SPIDATA::ReadByte() -> u8 {
  return value;
}

void SPI::SPIDATA::WriteByte(u8 value) {
  if(spi.spicnt.enable && spi.spicnt.bugged_hword_mode) {
    ATOM_PANIC("SPI: bugged 16-bit mode is currently unimplemented.");
  }

  if (!spi.spicnt.enable) {
    ATOM_ERROR("SPI: attempted to write SPIDATA while SPI bus is disabled.");
    return;
  }

  if (spi.spicnt.enable_irq) {
    spi.irq7.Raise(IRQ::Source::SPI);
  }

  auto device = spi.devices[spi.spicnt.device];

  if (device == nullptr) {
    ATOM_WARN("SPI: attempted to access unimplemented or reserved device.");
    return;
  }

  this->value = device->Transfer(value);

  if (!spi.spicnt.chipselect_hold) {
    device->Deselect();
    device->Select();
  }
}

} // namespace lunar::nds
