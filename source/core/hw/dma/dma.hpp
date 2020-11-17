/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <core/arm/memory.hpp>
#include <core/hw/irq/irq.hpp>

namespace fauxDS::core {

struct DMA {
  using Bus = arm::MemoryBase::Bus;

  DMA(IRQ& irq) : irq(irq) {
    Reset();
  }

  void Reset();
  auto Read (uint chan_id, uint offset) -> u8;
  void Write(uint chan_id, uint offset, u8 value);
  auto ReadFill (uint offset) -> u8;
  void WriteFill(uint offset, u8 value);

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
    bool enable = false;
    bool repeat = false;
    bool interrupt = false;

    u32 length = 0;
    u32 dst = 0;
    u32 src = 0;

    enum AddressMode {
      Increment = 0,
      Decrement = 1,
      Fixed  = 2,
      Reload = 3
    } dst_mode = Increment, src_mode = Increment;

    // TODO: how to handle NDS7 and NDS9 differences?
    enum Timing {
      Immediate = 0
    } time = Immediate;

    enum Size {
      Half = 0,
      Word = 1
    } size = Half;
  } channels[4];

  u8 filldata[16];
  arm::MemoryBase* memory;
  IRQ& irq;
};

} // namespace fauxDS::core
