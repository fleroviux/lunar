/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/integer.hpp>
#include <array>
#include <lunar/device/input_device.hpp>
#include <functional>
#include <string.h>
#include <vector>

#include "common/scheduler.hpp"
#include "nds/arm7/apu/apu.hpp"
#include "nds/arm7/dma/dma.hpp"
#include "nds/arm7/rtc/rtc.hpp"
#include "nds/arm7/spi/spi.hpp"
#include "nds/arm7/wifi/wifi.hpp"
#include "nds/arm9/dma/dma.hpp"
#include "nds/arm9/math/math.hpp"
#include "nds/cart/cart.hpp"
#include "nds/ipc/ipc.hpp"
#include "nds/irq/irq.hpp"
#include "nds/keypad/keypad.hpp"
#include "nds/timer/timer.hpp"
#include "nds/video_unit/video_unit.hpp"
#include "exmemcnt.hpp"
#include "swram.hpp"

namespace lunar::nds {

struct Interconnect {
  Interconnect()
      : apu(scheduler)
      , cart(scheduler, irq7, irq9, dma7, dma9, exmemcnt) 
      , ipc(irq7, irq9)
      , spi(irq7)
      , timer7(scheduler, irq7)
      , timer9(scheduler, irq9)
      , dma7(irq7)
      , dma9(irq9)
      , video_unit(scheduler, irq7, irq9, dma7, dma9)
      , keypad(irq7, irq9) {
    Reset();
  }

  void Reset() {    
    scheduler.Reset();
    apu.Reset();
    cart.Reset();
    irq7.Reset();
    irq9.Reset();
    math.Reset();
    ipc.Reset();
    spi.Reset();
    timer7.Reset();
    timer9.Reset();
    dma7.Reset();
    dma9.Reset();
    video_unit.Reset();
    wifi.Reset();
    rtc.Reset();
    swram.Reset();
    keypad.Reset();
    exmemcnt = {};
    ewram.fill(0);
  }

  void SetInputDevice(InputDevice& device) {
    keypad.SetInputDevice(device);
    spi.tsc.SetInputDevice(device);
  }

  std::array<u8, 0x400000> ewram;

  SWRAM swram;

  Scheduler scheduler;
  APU apu;
  Cartridge cart;
  IRQ irq7;
  IRQ irq9;
  Math math;
  IPC ipc;
  SPI spi;
  Timer timer7;
  Timer timer9;
  DMA7 dma7;
  DMA9 dma9;
  VideoUnit video_unit;
  WIFI wifi;
  RTC rtc;
  KeyPad keypad;
  EXMEMCNT exmemcnt;
};

} // namespace lunar::nds
