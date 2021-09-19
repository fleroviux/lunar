/*
 * Copyright (C) 2021 fleroviux
 */

#include "arm9.hpp"

namespace Duality::Core {

ARM9::ARM9(Interconnect& interconnect)
    : bus(&interconnect)
    , cp15(&bus)
    // , core(arm::ARM::Architecture::ARMv5TE, &bus)
    , irq(interconnect.irq9) {
  // core.AttachCoprocessor(15, &cp15);
  // irq.SetCore(core);
  // interconnect.dma9.SetMemory(&bus);
  // Reset(0);

  core = lunatic::CreateCPU(lunatic::CPU::Descriptor{
    .memory = bus,
    .coprocessors = {
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, &cp15
    },
    .exception_base = 0xFFFF'0000
  });

  cp15.SetCore(core.get());
  irq.SetCoreJIT(core.get());
  interconnect.dma9.SetMemory(&bus);
  Reset(0);
}

void ARM9::Reset(u32 entrypoint) {
  // core.ExceptionBase(0xFFFF0000);
  // core.Reset();
  // core.SwitchMode(arm::MODE_SYS);
  // core.GetState().r13 = 0x03002F7C;
  // core.GetState().bank[arm::BANK_IRQ][arm::BANK_R13] = 0x03003F80;
  // core.GetState().bank[arm::BANK_SVC][arm::BANK_R13] = 0x03003FC0;
  // core.SetPC(entrypoint);

  // Explicitly reset CP15 because right now the JIT won't do that.
  cp15.Reset();

  // core->ExceptionBase(0xFFFF0000);
  // core->Reset();
  core->GetCPSR().f.mode = lunatic::Mode::System;
  core->GetGPR(lunatic::GPR::SP) = 0x03002F7C;
  core->GetGPR(lunatic::GPR::SP, lunatic::Mode::IRQ) = 0x03003F80;
  core->GetGPR(lunatic::GPR::SP, lunatic::Mode::Supervisor) = 0x03003FC0;
  core->GetGPR(lunatic::GPR::PC) = entrypoint + sizeof(u32) * 2;
}

void ARM9::Run(uint cycles) {
  // core.Run(cycles);

  core->Run(cycles);
}

} // namespace Duality::Core
