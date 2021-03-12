/*
 * Copyright (C) 2021 fleroviux
 */

#include "arm9.hpp"

namespace Duality::Core {

ARM9::ARM9(Interconnect& interconnect)
    : bus(&interconnect)
    , cp15(&core, &bus)
    , core(arm::ARM::Architecture::ARMv5TE, &bus)
    , irq(interconnect.irq9) {
  core.AttachCoprocessor(15, &cp15);
  irq.SetCore(core);
  interconnect.dma9.SetMemory(&bus);
  Reset(0);
}

void ARM9::Reset(u32 entrypoint) {
  core.ExceptionBase(0xFFFF0000);
  core.Reset();
  core.GetState().r13 = 0x03002F7C;
  core.GetState().bank[arm::BANK_IRQ][arm::BANK_R13] = 0x03003F80;
  core.GetState().bank[arm::BANK_SVC][arm::BANK_R13] = 0x03003FC0;
  core.SetPC(entrypoint);
}

void ARM9::Run(uint cycles) {
  core.Run(cycles);
}

} // namespace Duality::Core
