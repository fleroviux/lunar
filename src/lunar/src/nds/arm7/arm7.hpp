/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/cpu.hpp>

#include "nds/interconnect.hpp"
#include "bus/bus.hpp"

namespace lunar::nds {

class ARM7 {
  public:
    explicit ARM7(Interconnect& interconnect);

    void Reset(u32 entrypoint);
    auto Bus() -> ARM7MemoryBus& { return bus; }
    bool IsHalted() { return bus.IsHalted(); }
    void Run(uint cycles);

  private:
    struct CP14 : lunatic::Coprocessor {
      void Reset() override {}

      auto Read(
        int opcode1,
        int cn,
        int cm,
        int opcode2
      ) -> u32 override { return 0; }

      void Write(
        int opcode1,
        int cn,
        int cm,
        int opcode2,
        u32 value
      ) override {}
    } cp14;

    ARM7MemoryBus bus;
    std::unique_ptr<lunatic::CPU> core;
    IRQ& irq;
};

} // namespace lunar::nds
