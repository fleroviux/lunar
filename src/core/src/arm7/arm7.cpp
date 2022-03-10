/*
 * Copyright (C) 2021 fleroviux
 */

#include "arm7.hpp"

namespace Duality::Core {

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
    using namespace lunatic;

    using Bus = Memory::Bus;

    bus.IsHalted() = false;
    bus.debug_r14 = core->GetGPR(GPR::LR);
    bus.debug_r15 = core->GetGPR(GPR::PC);

    const u32 card_send_cmd = 0x00001698;
    const u32 card_send_cmd_and_rx_data = 0x000016d8;
    const u32 card_irq_handler = 0x00001dc4;
    const u32 card_key1_decrypt_secure_area_and_verify = 0x000027F8;

    #define t(addr) (addr + 4)

    // Intercept and log calls to card_send_cmd
    /*if (bus.debug_r15 == t(card_send_cmd)) {
      auto p_cmd_info = core->GetGPR(GPR::R0);
      auto dma_id = bus.ReadWord(p_cmd_info, Bus::Data);
      auto cmd_lo = bus.ReadWord(p_cmd_info +  8, Bus::Data);
      auto cmd_hi = bus.ReadWord(p_cmd_info + 12, Bus::Data);

      LOG_TRACE("ARM7: Cart: card_send_cmd() dma_id={} cmd=0x{:08X}{:08X} lr=0x{:08X}", dma_id, cmd_hi, cmd_lo, bus.debug_r14);
    }

    if (bus.debug_r15 == t(card_send_cmd_and_rx_data)) {
      LOG_TRACE("ARM7: Cart: card_send_cmd_and_rx_data() lr=0x{:08X}", bus.debug_r14);
    }

    if (bus.debug_r15 == t(card_irq_handler)) {
      LOG_TRACE("ARM7: Cart: card_irq_handler() lr=0x{:08X}", bus.debug_r14);
    }*/

    if (bus.debug_r15 == t(0x00002866)) {
      LOG_TRACE("ARM7: Cart: encryObj test PASSED :3");
    }

    if (bus.debug_r15 == t(0x00002880)) {
      LOG_TRACE("ARM7: Cart: encryObj test FAILED! :C");
    }

    /*if (bus.debug_r15 == t(0x0000285a)) {
      LOG_TRACE("ARM7: Cart: encrypObj check0 r1=0x{:08X} r3=0x{:08X}", core->GetGPR(GPR::R1), core->GetGPR(GPR::R3));
    }

    if (bus.debug_r15 == t(0x00002862)) {
      LOG_TRACE("ARM7: Cart: encrypObj check1 r1=0x{:08X} r2=0x{:08X}", core->GetGPR(GPR::R1), core->GetGPR(GPR::R2));
    
      core->SetGPR(GPR::R1, 0x6A624F79);
    }*/

    if (bus.debug_r15 == t(card_key1_decrypt_secure_area_and_verify)) {
      LOG_TRACE("ARM7: Cart: card_key1_decrypt_secure_area_and_verify() lr=0x{:08X}", bus.debug_r14);
    }

    core->Run(cycles);
  }
}

} // namespace Duality::Core
