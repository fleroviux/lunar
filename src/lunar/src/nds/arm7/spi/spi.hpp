/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/integer.hpp>

#include "tsc/tsc.hpp"
#include "nds/cart/backup/flash.hpp"
#include "nds/irq/irq.hpp"

namespace lunar::nds {

// Serial Peripheral Interface
class SPI {
  public:
    explicit SPI(IRQ& irq7);

    void Reset();

    FLASH firmware;
    TSC tsc;

    struct SPICNT {
      explicit SPICNT(SPI& spi) : spi(spi) {}

      auto ReadByte (uint offset) -> u8;
      void WriteByte(uint offset, u8 value);

    private:
      friend struct lunar::nds::SPI;

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
      explicit SPIDATA(SPI& spi) : spi(spi) {}

      auto ReadByte () -> u8;
      void WriteByte(u8 value);
    private:
      friend struct lunar::nds::SPI;

      u8 value = 0;
      SPI& spi;
    } spidata { *this };

  private:
    IRQ& irq7;
    SPIDevice* devices[4] { nullptr, &firmware, &tsc, nullptr };
};

} // namespace lunar::nds
