/*
 * Copyright (C) 2020 fleroviux
 */

#include <stdexcept>

#include "arm.hpp"

namespace Duality::Core::arm {

ARM::ARM(lunatic::CPU::Descriptor const& descriptor)
    : arch(Architecture(descriptor.model))
    , exception_base(descriptor.exception_base)
    , memory(&descriptor.memory)
    , coprocessors(descriptor.coprocessors) {
  BuildConditionTable();
  Reset();
}

void ARM::Reset() {
  constexpr u32 nop = 0xE320F000;

  state.Reset();
  SwitchMode(state.cpsr.f.mode);
  opcode[0] = nop;
  opcode[1] = nop;
  state.r15 = exception_base;
  wait_for_irq = false;
  IRQLine() = false;
}

auto ARM::IRQLine() -> bool& {
  return irq_line;
}

auto ARM::WaitForIRQ() -> bool& {
  return wait_for_irq;
}

auto ARM::Run(int cycles) -> int {
  if (WaitForIRQ() && !IRQLine()) {
    return 0;
  }

  while (cycles-- > 0) {
    if (IRQLine()) SignalIRQ();

    auto instruction = opcode[0];
    if (state.cpsr.f.thumb) {
      state.r15 &= ~1;

      opcode[0] = opcode[1];
      opcode[1] = ReadHalfCode(state.r15);
      (this->*s_opcode_lut_16[instruction >> 5])(instruction);
    } else {
      state.r15 &= ~3;

      opcode[0] = opcode[1];
      opcode[1] = ReadWordCode(state.r15);
      auto condition = static_cast<Condition>(instruction >> 28);
      if (CheckCondition(condition)) {
        int hash = ((instruction >> 16) & 0xFF0) |
                   ((instruction >>  4) & 0x00F);
        if (condition == COND_NV) {
          hash |= 4096;
        }
        (this->*s_opcode_lut_32[hash])(instruction);

        if (WaitForIRQ()) return 0;
      } else {
        state.r15 += 4;
      }
    }
  }

  return 0;
}

auto ARM::GetGPR(lunatic::GPR reg) const -> u32 {
  return state.reg[int(reg)];
}

auto ARM::GetGPR(lunatic::GPR reg, lunatic::Mode mode) const -> u32 {
  auto reg_id = int(reg);
  auto limit = Mode(mode) == MODE_FIQ ? 8 : 13;
  auto current_mode = state.cpsr.f.mode;

  if (current_mode != Mode(mode) && reg_id >= limit && reg_id != 15) {
    return state.bank[GetRegisterBankByMode(Mode(mode))][reg_id - 8];
  }

  return state.reg[reg_id];
}

auto ARM::GetCPSR() const -> lunatic::StatusRegister {
  return {.v = state.cpsr.v};
}

auto ARM::GetSPSR(lunatic::Mode mode) const -> lunatic::StatusRegister {
  return {.v = state.spsr[GetRegisterBankByMode(Mode(mode))].v};
}

void ARM::SetGPR(lunatic::GPR reg, u32 value) {
  if (reg == lunatic::GPR::PC) {
    state.r15 = value;
    if (state.cpsr.f.thumb) {
      ReloadPipeline16();
    } else {
      ReloadPipeline32();
    }
  } else {
    state.reg[int(reg)] = value;
  }
}

void ARM::SetGPR(lunatic::GPR reg, lunatic::Mode mode, u32 value) {
  if (reg == lunatic::GPR::PC) {
    SetGPR(lunatic::GPR::PC, value);
  } else {
    auto reg_id = int(reg);
    auto limit = Mode(mode) == MODE_FIQ ? 8 : 13;
    auto current_mode = state.cpsr.f.mode;

    if (current_mode != Mode(mode) && reg_id >= limit) {
      auto bank = GetRegisterBankByMode(Mode(mode));
      state.bank[bank][reg_id - 8] = value;
    } else {
      state.reg[reg_id] = value;
    }
  }
}

void ARM::SetCPSR(lunatic::StatusRegister psr) {
  state.cpsr.v = psr.v;
}

void ARM::SetSPSR(lunatic::Mode mode, lunatic::StatusRegister psr) {
  state.spsr[GetRegisterBankByMode(Mode(mode))].v = psr.v;
}

void ARM::SignalIRQ() {
  wait_for_irq = false;

  if (state.cpsr.f.mask_irq) {
    return;
  }

  // Save current program status register.
  state.spsr[BANK_IRQ].v = state.cpsr.v;

  // Enter IRQ mode and disable IRQs.
  SwitchMode(MODE_IRQ);
  state.cpsr.f.mask_irq = 1;

  // Save current program counter and disable Thumb.
  if (state.cpsr.f.thumb) {
    state.cpsr.f.thumb = 0;
    state.r14 = state.r15;
  } else {
    state.r14 = state.r15 - 4;
  }
  
  // Jump to IRQ exception vector.
  state.r15 = exception_base + 0x18;
  ReloadPipeline32();
}

void ARM::ReloadPipeline32() {
  opcode[0] = ReadWordCode(state.r15);
  opcode[1] = ReadWordCode(state.r15 + 4);
  state.r15 += 8;
}

void ARM::ReloadPipeline16() {
  opcode[0] = ReadHalfCode(state.r15);
  opcode[1] = ReadHalfCode(state.r15 + 2);
  state.r15 += 4;
}

void ARM::BuildConditionTable() {
  for (int flags = 0; flags < 16; flags++) {
    bool n = flags & 8;
    bool z = flags & 4;
    bool c = flags & 2;
    bool v = flags & 1;

    condition_table[COND_EQ][flags] = z;
    condition_table[COND_NE][flags] = !z;
    condition_table[COND_CS][flags] =  c;
    condition_table[COND_CC][flags] = !c;
    condition_table[COND_MI][flags] =  n;
    condition_table[COND_PL][flags] = !n;
    condition_table[COND_VS][flags] =  v;
    condition_table[COND_VC][flags] = !v;
    condition_table[COND_HI][flags] =  c && !z;
    condition_table[COND_LS][flags] = !c ||  z;
    condition_table[COND_GE][flags] = n == v;
    condition_table[COND_LT][flags] = n != v;
    condition_table[COND_GT][flags] = !(z || (n != v));
    condition_table[COND_LE][flags] =  (z || (n != v));
    condition_table[COND_AL][flags] = true;
    condition_table[COND_NV][flags] = true;
  }
}

bool ARM::CheckCondition(Condition condition) {
  if (condition == COND_AL) {
    return true;
  }
  return condition_table[condition][state.cpsr.v >> 28];
}

auto ARM::GetRegisterBankByMode(Mode mode) -> Bank {
  switch (mode) {
    case MODE_USR:
    case MODE_SYS:
      return BANK_NONE;
    case MODE_FIQ:
      return BANK_FIQ;
    case MODE_IRQ:
      return BANK_IRQ;
    case MODE_SVC:
      return BANK_SVC;
    case MODE_ABT:
      return BANK_ABT;
    case MODE_UND:
      return BANK_UND;
  }

  UNREACHABLE;
}

void ARM::SwitchMode(Mode new_mode) {
  auto old_bank = GetRegisterBankByMode(state.cpsr.f.mode);
  auto new_bank = GetRegisterBankByMode(new_mode);

  state.cpsr.f.mode = new_mode;
  p_spsr = &state.spsr[new_bank];

  if (old_bank == new_bank) {
    return;
  }

  if (old_bank == BANK_FIQ || new_bank == BANK_FIQ) {
    for (int i = 0; i < 7; i++){
      state.bank[old_bank][i] = state.reg[8 + i];
    }

    for (int i = 0; i < 7; i++) {
      state.reg[8 + i] = state.bank[new_bank][i];
    }
  } else {
    state.bank[old_bank][5] = state.r13;
    state.bank[old_bank][6] = state.r14;

    state.r13 = state.bank[new_bank][5];
    state.r14 = state.bank[new_bank][6];
  }
}

} // namespace Duality::Core::arm
