/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "arm/arm.hpp"
#include "arm9.hpp"
#include "buildconfig.hpp"

namespace lunar::nds {

ARM9::ARM9(Interconnect& interconnect)
    : bus(&interconnect)
    , cp15(&bus)
    , irq(interconnect.irq9) {
//  auto cpu_descriptor = lunatic::CPU::Descriptor{
//    .memory = bus,
//    .coprocessors = {
//      nullptr, nullptr, nullptr, nullptr,
//      nullptr, nullptr, nullptr, nullptr,
//      nullptr, nullptr, nullptr, nullptr,
//      nullptr, nullptr, nullptr, &cp15
//    },
//    .exception_base = 0xFFFF'0000
//  };

  std::array<aura::arm::Coprocessor*, 16> coprocessors{
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, &cp15
  };

  core = std::make_unique<aura::arm::ARM>(&bus, aura::arm::CPU::Model::ARM11, coprocessors);
  core->SetExceptionBase(0xFFFF'0000);

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
  cpsr.mode = aura::arm::CPU::Mode::System;
  core->SetCPSR(cpsr);
  core->SetGPR(aura::arm::CPU::GPR::SP, 0x03002F7C);
  core->SetGPR(aura::arm::CPU::GPR::SP, aura::arm::CPU::Mode::IRQ, 0x03003F80);
  core->SetGPR(aura::arm::CPU::GPR::SP, aura::arm::CPU::Mode::Supervisor, 0x03003FC0);
  core->SetGPR(aura::arm::CPU::GPR::PC, entrypoint);
}

void ARM9::Run(uint cycles) {
  core->Run(cycles);
}

} // namespace lunar::nds
