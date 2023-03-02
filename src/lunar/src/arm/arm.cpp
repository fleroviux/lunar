
#include "arm.hpp"

namespace aura::arm {

ARM::ARM(
  Memory* memory,
  Model model,
  std::array<Coprocessor*, 16> coprocessors
)   : memory{memory}
    , model{model}
    , coprocessors{coprocessors} {
  BuildConditionTable();
  Reset();
}

void ARM::Reset() {
  constexpr u32 nop = 0xE320F000;

  state = {};
  SwitchMode((Mode)state.cpsr.mode);
  opcode[0] = nop;
  opcode[1] = nop;
  state.r15 = exception_base;
  wait_for_irq = false;
  SetIRQFlag(false);
}

void ARM::Run(int cycles) {
  if (GetWaitingForIRQ() && !GetIRQFlag()) {
    return;
  }

  while (cycles-- > 0) {
    if (GetIRQFlag()) SignalIRQ();

    auto instruction = opcode[0];
    if (state.cpsr.thumb) {
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
        if (condition == Condition::NV) {
          hash |= 4096;
        }
        (this->*s_opcode_lut_32[hash])(instruction);

        if (GetWaitingForIRQ()) return;
      } else {
        state.r15 += 4;
      }
    }
  }
}

void ARM::SignalIRQ() {
  wait_for_irq = false;

  if (state.cpsr.mask_irq) {
    return;
  }

  // Save current program status register.
  state.spsr[(int)Bank::IRQ] = state.cpsr;

  // Enter IRQ mode and disable IRQs.
  SwitchMode(Mode::IRQ);
  state.cpsr.mask_irq = 1;

  // Save current program counter and disable Thumb.
  if (state.cpsr.thumb) {
    state.cpsr.thumb = 0;
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

    condition_table[(int)Condition::EQ][flags] = z;
    condition_table[(int)Condition::NE][flags] = !z;
    condition_table[(int)Condition::CS][flags] =  c;
    condition_table[(int)Condition::CC][flags] = !c;
    condition_table[(int)Condition::MI][flags] =  n;
    condition_table[(int)Condition::PL][flags] = !n;
    condition_table[(int)Condition::VS][flags] =  v;
    condition_table[(int)Condition::VC][flags] = !v;
    condition_table[(int)Condition::HI][flags] =  c && !z;
    condition_table[(int)Condition::LS][flags] = !c ||  z;
    condition_table[(int)Condition::GE][flags] = n == v;
    condition_table[(int)Condition::LT][flags] = n != v;
    condition_table[(int)Condition::GT][flags] = !(z || (n != v));
    condition_table[(int)Condition::LE][flags] =  (z || (n != v));
    condition_table[(int)Condition::AL][flags] = true;
    condition_table[(int)Condition::NV][flags] = true;
  }
}

bool ARM::CheckCondition(Condition condition) {
  if (condition == Condition::AL) {
    return true;
  }
  return condition_table[(int)condition][state.cpsr.word >> 28];
}

auto ARM::GetRegisterBankByMode(Mode mode) -> Bank {
  switch (mode) {
    case Mode::User:
    case Mode::System: {
      return Bank::None;
    }
    case Mode::FIQ: {
      return Bank::FIQ;
    }
    case Mode::IRQ: {
      return Bank::IRQ;
    }
    case Mode::Supervisor: {
      return Bank::Supervisor;
    }
    case Mode::Abort: {
      return Bank::Abort;
    }
    case Mode::Undefined: {
      return Bank::Undefined;
    }
  }

  ATOM_PANIC("invalid ARM CPU mode: 0x{:02X}", (uint)mode);
}

void ARM::SwitchMode(Mode new_mode) {
  auto old_bank = GetRegisterBankByMode((Mode)state.cpsr.mode);
  auto new_bank = GetRegisterBankByMode(new_mode);

  state.cpsr.mode = new_mode;
  p_spsr = &state.spsr[(int)new_bank];

  if (old_bank == new_bank) {
    return;
  }

  if (old_bank == Bank::FIQ) {
    for (int i = 0; i < 5; i++){
      state.bank[(int)Bank::FIQ][i] = state.reg[8 + i];
    }

    for (int i = 0; i < 5; i++) {
      state.reg[8 + i] = state.bank[(int)Bank::None][i];
    }
  } else if(new_bank == Bank::FIQ) {
    for (int i = 0; i < 5; i++) {
      state.bank[(int)Bank::None][i] = state.reg[8 + i];
    }

    for (int i = 0; i < 5; i++) {
      state.reg[8 + i] = state.bank[(int)Bank::FIQ][i];
    }
  }

  state.bank[(int)old_bank][5] = state.r13;
  state.bank[(int)old_bank][6] = state.r14;

  state.r13 = state.bank[(int)new_bank][5];
  state.r14 = state.bank[(int)new_bank][6];
}

} // namespace aura::arm
