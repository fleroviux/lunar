/*
 * Copyright (C) fleroviux
 */

#pragma once

#include <common/integer.hpp>

#include "tsc/tsc.hpp"

namespace fauxDS::core {

/// Serial Peripheral Interface
struct SPI {
  SPI();

  void Reset();

  struct SPICNT {
    SPICNT(SPI& spi) : spi(spi) {}

    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct fauxDS::core::SPI;

    int  baudrate = 0;
    bool busy = false;
    //Device device = Device::Powerman;
    int  device = 3;
    bool bugged_hword_mode = false;
    bool chipselect_hold = false;
    bool enable_irq = false;
    bool enable = false;

    SPI& spi;
  } spicnt { *this };

  struct SPIDATA {
    SPIDATA(SPI& spi) : spi(spi) {}

    auto ReadByte () -> u8;
    void WriteByte(u8 value);
  private:
    friend struct fauxDS::core::SPI;

    u8 value = 0;
    SPI& spi;
  } spidata { *this };

private:
  TSC tsc;

  SPIDevice* devices[4] { nullptr, nullptr, &tsc, nullptr };
};

} // namespace fauxDS::core
