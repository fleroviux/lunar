/*
 * Copyright (C) 2021 fleroviux
 */

#include "arm7.hpp"

namespace Duality::Core {

ARM7::ARM7(Interconnect& interconnect)
    : bus(&interconnect)
    // , core(arm::ARM::Architecture::ARMv4T, &bus)
    , irq(interconnect.irq7) {
  // core.AttachCoprocessor(14, &cp14);
  // irq.SetCore(core);
  // interconnect.dma7.SetMemory(&bus);
  // interconnect.apu.SetMemory(&bus);
  // Reset(0);

  core = lunatic::CreateCPU(lunatic::CPU::Descriptor{
    .memory = bus,
    .coprocessors = {
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, &cp14,   nullptr 
    },
    .exception_base = 0x0000'0000
  });

  irq.SetCoreJIT(core.get());
  interconnect.dma7.SetMemory(&bus);
  interconnect.apu.SetMemory(&bus);
  Reset(0);
}

void ARM7::Reset(u32 entrypoint) {
  // TODO: reset the bus and all associated devices.
  
  // core.ExceptionBase(0);
  // core.Reset();
  // core.SwitchMode(arm::MODE_SYS);
  // core.GetState().r13 = 0x0380FD80;
  // core.GetState().bank[arm::BANK_IRQ][arm::BANK_R13] = 0x0380FF80;
  // core.GetState().bank[arm::BANK_SVC][arm::BANK_R13] = 0x0380FFC0;
  // core.SetPC(entrypoint);

  // core->ExceptionBase(0);
  // core->Reset();
  core->GetCPSR().f.mode = lunatic::Mode::System;
  core->GetGPR(lunatic::GPR::SP) = 0x0380FD80;
  core->GetGPR(lunatic::GPR::SP, lunatic::Mode::IRQ) = 0x0380FF80;
  core->GetGPR(lunatic::GPR::SP, lunatic::Mode::Supervisor) = 0x0380FFC0;
  core->GetGPR(lunatic::GPR::PC) = entrypoint + sizeof(u32) * 2;
}

void ARM7::Run(uint cycles) {
  if (!bus.IsHalted() || irq.HasPendingIRQ()) {
    bus.IsHalted() = false;
    // core.Run(cycles);
    core->Run(cycles);
  }
}

} // namespace Duality::Core
