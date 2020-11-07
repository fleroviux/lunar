/*
 * Copyright (C) fleroviux
 */

#pragma once

#include <common/integer.hpp>

namespace fauxDS::core {

/// Serial Peripheral Interface
struct SPI {
  enum class Device : uint {
    Powerman = 0,
    Firmware = 1,
    Touchscreen = 2,
    Reserved = 3
  };

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
    Device device = Device::Powerman;
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

};

} // namespace fauxDS::core
