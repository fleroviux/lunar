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
#include "cp15.hpp"

namespace lunar::nds {

struct ARM9 {
  ARM9(Interconnect& interconnect);

  void Reset(u32 entrypoint);
  auto Bus() -> ARM9MemoryBus& { return bus; }
  bool IsHalted() { return core->WaitForIRQ(); }
  void Run(uint cycles);

private:
  ARM9MemoryBus bus;
  CP15 cp15;
  std::unique_ptr<lunatic::CPU> core;
  IRQ& irq;
};

} // namespace lunar::nds
