/*
 * Copyright (C) 2021 fleroviux
 */

#include "arm/arm.hpp"
#include "arm9.hpp"

namespace Duality::Core {

ARM9::ARM9(Interconnect& interconnect)
    : bus(&interconnect)
    , cp15(&bus)
    , irq(interconnect.irq9) {
  auto cpu_descriptor = lunatic::CPU::Descriptor{
    .memory = bus,
    .coprocessors = {
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, &cp15
    },
    .exception_base = 0xFFFF'0000
  };

  if (gEnableJITRecompiler) {
    core = lunatic::CreateCPU(cpu_descriptor);
  } else {
    core = std::make_unique<arm::ARM>(cpu_descriptor);
  }

  cp15.SetCore(core.get());
  irq.SetCore(core.get());
  interconnect.dma9.SetMemory(&bus);
  Reset(0);
}

void ARM9::Reset(u32 entrypoint) {
  // TODO: reset the bus and all associated devices.
  core->Reset();
  // core->ExceptionBase(0xFFFF0000);
  cp15.Reset();

  auto cpsr = core->GetCPSR();
  cpsr.f.mode = lunatic::Mode::System;
  core->SetCPSR(cpsr);
  core->SetGPR(lunatic::GPR::SP, 0x03002F7C);
  core->SetGPR(lunatic::GPR::SP, lunatic::Mode::IRQ, 0x03003F80);
  core->SetGPR(lunatic::GPR::SP, lunatic::Mode::Supervisor, 0x03003FC0);
  core->SetGPR(lunatic::GPR::PC, entrypoint);
}

void ARM9::Run(uint cycles) {
  core->Run(cycles);
}

} // namespace Duality::Core
