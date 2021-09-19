/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <lunatic/cpu.hpp>

// #include "arm/arm.hpp"
#include "interconnect.hpp"
#include "bus.hpp"
#include "cp15.hpp"

namespace Duality::Core {

struct ARM9 {
  ARM9(Interconnect& interconnect);

  void Reset(u32 entrypoint);
  auto Bus() -> ARM9MemoryBus& { return bus; }
  bool IsHalted() { return core->IsWaitingForIRQ(); }
  void Run(uint cycles);

private:
  ARM9MemoryBus bus;
  CP15 cp15;
  // arm::ARM core;
  std::unique_ptr<lunatic::CPU> core;
  IRQ& irq;
};

} // namespace Duality::Core
