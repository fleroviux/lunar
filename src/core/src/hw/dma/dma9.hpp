/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <util/integer.hpp>

#include "arm/memory.hpp"
#include "hw/irq/irq.hpp"

namespace Duality::core {

struct DMA9 {
  enum Time {
    Immediate = 0,
    VBlank = 1,
    HBlank = 2,
    VideoTransfer = 3,
    MainMemoryDisplay = 4,
    Slot1 = 5,
    Slot2 = 6,
    GxFIFO = 7
  };

  using Bus = arm::MemoryBase::Bus;

  DMA9(IRQ& irq) : irq(irq) {
    Reset();
  }

  void Reset();
  auto Read (uint chan_id, uint offset) -> u8;
  void Write(uint chan_id, uint offset, u8 value);
  auto ReadFill (uint offset) -> u8;
  void WriteFill(uint offset, u8 value);
  void Request(Time time);
  void SetGXFIFOHalfEmpty(bool value) { gxfifo_half_empty = value; }

  // TODO: get rid of this ugly hack that only exists
  // because we can't pass "memory" to the constructor at the moment.
  void SetMemory(arm::MemoryBase* memory) { this->memory = memory; }

private:
  enum Registers {
    REG_DMAXSAD = 0,
    REG_DMAXDAD = 4,
    REG_DMAXCNT_L = 8,
    REG_DMAXCNT_H = 10
  };

  struct Channel {
    Channel(uint id) : id(id) {}

    uint id;
    bool enable = false;
    bool repeat = false;
    bool interrupt = false;
    bool running = false;

    u32 length = 0;
    u32 dst = 0;
    u32 src = 0;

    enum AddressMode {
      Increment = 0,
      Decrement = 1,
      Fixed  = 2,
      Reload = 3
    } dst_mode = Increment, src_mode = Increment;

    Time time = Immediate;

    enum Size {
      Half = 0,
      Word = 1
    } size = Half;

    struct {
      u32 length;
      u32 dst;
      u32 src;
    } latch;
  } channels[4] { 0, 1, 2, 3 };

  void RunChannel(Channel& channel);

  u8 filldata[16];
  arm::MemoryBase* memory;
  IRQ& irq;
  bool gxfifo_half_empty;
};

} // namespace Duality::core
