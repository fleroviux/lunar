/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <lunatic/cpu.hpp>

#include "interconnect.hpp"
#include "bus.hpp"

namespace Duality::Core {

struct ARM7 {
  ARM7(Interconnect& interconnect);

  void Reset(u32 entrypoint);
  auto Bus() -> ARM7MemoryBus& { return bus; }
  bool IsHalted() { return bus.IsHalted(); }
  void Run(uint cycles);

private:
  /// No-operation stub for the CP14 coprocessor
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

} // namespace Duality::Core
