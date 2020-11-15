/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>
#include <core/hw/cart/cart.hpp>
#include <core/hw/dma/dma.hpp>
#include <core/hw/ipc/ipc.hpp>
#include <core/hw/irq/irq.hpp>
#include <core/hw/spi/spi.hpp>
#include <core/hw/timer/timer.hpp>
#include <core/hw/video_unit/video_unit.hpp>
#include <string.h>

#include "scheduler.hpp"

namespace fauxDS::core {

struct Interconnect {
  Interconnect()
      : ipc(irq7, irq9)
      , spi(irq7)
      , timer7(scheduler, irq7)
      , timer9(scheduler, irq9)
      , dma7(irq7)
      , dma9(irq9)
      , video_unit(&scheduler, irq7, irq9)
      , wramcnt(swram) {
    Reset();
  }

  void Reset() {
    memset(ewram, 0, sizeof(ewram));
    memset(swram.data, 0, sizeof(swram));
    
    scheduler.Reset();
    cart.Reset();
    irq7.Reset();
    irq9.Reset();
    ipc.Reset();
    spi.Reset();
    timer7.Reset();
    timer9.Reset();
    dma7.Reset();
    dma9.Reset();
    video_unit.Reset();

    // TODO: this is just a stub!
    swram.arm9 = { nullptr, 0 };
    swram.arm7 = { swram.data, 0x7FFF };
  }

  u8 ewram[0x400000];

  struct SWRAM {
    u8 data[0x8000];

    struct Alloc {
      u8* data = nullptr;
      u32 mask = 0;
    } arm9 = {}, arm7 = {};
  } swram;

  Scheduler scheduler;
  Cartridge cart;
  IRQ irq7;
  IRQ irq9;
  IPC ipc;
  SPI spi;
  Timer timer7;
  Timer timer9;
  DMA dma7;
  DMA dma9;
  VideoUnit video_unit;

  struct WRAMCNT {
    WRAMCNT(SWRAM& swram) : swram(swram) {}

    auto ReadByte() -> u8 {
      return value;
    }

    void WriteByte(u8 value) {
      this->value = value & 3;
      switch (this->value) {
        case 0:
          swram.arm9 = { swram.data, 0x7FFF };
          swram.arm7 = { nullptr, 0 };
          break;
        case 1:
          swram.arm9 = { &swram.data[0x4000], 0x3FFF };
          swram.arm7 = { &swram.data[0x0000], 0x3FFF };
          break;
        case 2:
          swram.arm9 = { &swram.data[0x0000], 0x3FFF };
          swram.arm7 = { &swram.data[0x4000], 0x3FFF };
          break;
        case 3:
          swram.arm9 = { nullptr, 0 };
          swram.arm7 = { swram.data, 0x7FFF };
          break;
      }
    }

  private:
    int value = 0;
    SWRAM& swram;
  } wramcnt;

  struct KeyInput {
    bool a = false;
    bool b = false;
    bool select = false;
    bool start = false;
    bool right = false;
    bool left = false;
    bool up = false;
    bool down = false;
    bool r = false;
    bool l = false;

    auto ReadByte(uint offset) -> u8 {
      switch (offset) {
        case 0:
          return (a      ? 0 :   1) |
                 (b      ? 0 :   2) |
                 (select ? 0 :   4) |
                 (start  ? 0 :   8) |
                 (right  ? 0 :  16) |
                 (left   ? 0 :  32) |
                 (up     ? 0 :  64) |
                 (down   ? 0 : 128);
        case 1:
          return (r ? 0 : 1) |
                 (l ? 0 : 2);
      }

      UNREACHABLE;
    }
  } keyinput = {};
};

} // namespace fauxDS::core
