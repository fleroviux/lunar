/*
 * Copyright (C) 2021 fleroviux
 */

#include "arm7.hpp"

namespace Duality::Core {

ARM7::ARM7(Interconnect& interconnect)
    : bus(&interconnect)
    , core(arm::ARM::Architecture::ARMv4T, &bus)
    , irq(interconnect.irq7) {
  core.AttachCoprocessor(14, &cp14);
  irq.SetCore(core);
  interconnect.dma7.SetMemory(&bus);
  interconnect.apu.SetMemory(&bus);
  Reset(0);
}

void ARM7::Reset(u32 entrypoint) {
  // TODO: reset the bus and all associated devices.
  core.ExceptionBase(0);
  core.Reset();
  core.SwitchMode(arm::MODE_SYS);
  core.GetState().r13 = 0x0380FD80;
  core.GetState().bank[arm::BANK_IRQ][arm::BANK_R13] = 0x0380FF80;
  core.GetState().bank[arm::BANK_SVC][arm::BANK_R13] = 0x0380FFC0;
  core.SetPC(entrypoint);
}

void ARM7::Run(uint cycles) {
  if (!bus.IsHalted() || irq.HasPendingIRQ()) {
    bus.IsHalted() = false;
    core.Run(cycles);
  }
}

} // namespace Duality::Core
