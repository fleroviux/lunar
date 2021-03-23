/*
 * Copyright (C) fleroviux
 */

#pragma once

#include <util/integer.hpp>

#include "tsc/tsc.hpp"
#include "hw/cart/backup/flash.hpp"
#include "hw/irq/irq.hpp"

namespace Duality::Core {

/// Serial Peripheral Interface
struct SPI {
  SPI(IRQ& irq7);

  void Reset();

  FLASH firmware;
  TSC tsc;

  struct SPICNT {
    SPICNT(SPI& spi) : spi(spi) {}

    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct Duality::Core::SPI;

    int  baudrate = 0;
    bool busy = false;
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
    friend struct Duality::Core::SPI;

    u8 value = 0;
    SPI& spi;
  } spidata { *this };

private:
  IRQ& irq7;
  SPIDevice* devices[4] { nullptr, &firmware, &tsc, nullptr };
};

} // namespace Duality::Core
