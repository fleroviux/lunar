/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/memory.hpp>
#include <atom/integer.hpp>

#include "nds/irq/irq.hpp"

namespace lunar::nds {

class DMA7 {
  public:
    enum Time {
      Immediate = 0,
      VBlank = 1,
      Slot1 = 2,
      Special = 3
    };

    using Bus = lunatic::Memory::Bus;

    explicit DMA7(IRQ& irq) : irq(irq) {
      Reset();
    }

    void Reset();
    auto Read (uint chan_id, uint offset) -> u8;
    void Write(uint chan_id, uint offset, u8 value);
    void Request(Time time);

    // TODO: get rid of this ugly hack that only exists
    // because we can't pass "memory" to the constructor at the moment.
    void SetMemory(lunatic::Memory* memory) { this->memory = memory; }

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

    lunatic::Memory* memory{};
    IRQ& irq;
};

} // namespace lunar::nds
