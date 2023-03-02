/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "arm/arm.hpp"
#include "arm7.hpp"
#include "buildconfig.hpp"

namespace lunar::nds {

ARM7::ARM7(Interconnect& interconnect)
    : bus(&interconnect)
    , irq(interconnect.irq7) {
//  auto cpu_descriptor = lunatic::CPU::Descriptor{
//    .memory = bus,
//    .coprocessors = {
//      nullptr, nullptr, nullptr, nullptr,
//      nullptr, nullptr, nullptr, nullptr,
//      nullptr, nullptr, nullptr, nullptr,
//      nullptr, nullptr, &cp14,   nullptr
//    },
//    .exception_base = 0x0000'0000
//  };

  const std::array<aura::arm::Coprocessor*, 16> coprocessors{
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, &cp14,   nullptr
  };

  core = std::make_unique<arm::ARM>(&bus, lunar::arm::ARM::Architecture::ARMv4T, coprocessors);

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
  cpsr.mode = aura::arm::CPU::Mode::System;
  core->SetCPSR(cpsr);
  core->SetGPR(aura::arm::CPU::GPR::SP, 0x0380FD80);
  core->SetGPR(aura::arm::CPU::GPR::SP, aura::arm::CPU::Mode::IRQ, 0x0380FF80);
  core->SetGPR(aura::arm::CPU::GPR::SP, aura::arm::CPU::Mode::Supervisor, 0x0380FFC0);
  core->SetGPR(aura::arm::CPU::GPR::PC, entrypoint);
}

void ARM7::Run(uint cycles) {
  if (!bus.IsHalted() || irq.HasPendingIRQ()) {
    bus.IsHalted() = false;
    core->Run(cycles);
  }
}

} // namespace lunar::nds
