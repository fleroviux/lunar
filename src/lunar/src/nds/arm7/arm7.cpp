
// Copyright (C) 2022 fleroviux

#include "arm/arm.hpp"
#include "arm7.hpp"
#include "buildconfig.hpp"

namespace lunar::nds {

ARM7::ARM7(Interconnect& interconnect)
    : bus(&interconnect)
    , irq(interconnect.irq7) {
  auto cpu_descriptor = lunatic::CPU::Descriptor{
    .memory = bus,
    .coprocessors = {
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, &cp14,   nullptr 
    },
    .exception_base = 0x0000'0000
  };

  if (gEnableJITRecompiler) {
    core = lunatic::CreateCPU(cpu_descriptor);
  } else {
    core = std::make_unique<arm::ARM>(cpu_descriptor);
  }

  irq.SetCore(core.get());
  interconnect.dma7.SetMemory(&bus);
  interconnect.apu.SetMemory(&bus);
  Reset(0);
}

void ARM7::Reset(u32 entrypoint) {
  // TODO: reset the bus and all associated devices.
  core->Reset();
  // core->ExceptionBase(0);

  auto cpsr = core->GetCPSR();
  cpsr.f.mode = lunatic::Mode::System;
  core->SetCPSR(cpsr);
  core->SetGPR(lunatic::GPR::SP, 0x0380FD80);
  core->SetGPR(lunatic::GPR::SP, lunatic::Mode::IRQ, 0x0380FF80);
  core->SetGPR(lunatic::GPR::SP, lunatic::Mode::Supervisor, 0x0380FFC0);
  core->SetGPR(lunatic::GPR::PC, entrypoint);
}

void ARM7::Run(uint cycles) {
  if (!bus.IsHalted() || irq.HasPendingIRQ()) {
    bus.IsHalted() = false;
    core->Run(cycles);
  }
}

} // namespace lunar::nds
