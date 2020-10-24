/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <array>
#include <fmt/format.h>

#include "cp15.hpp"
#include "state.hpp"
#include "memory.hpp"

namespace fauxDS::core::arm {

struct ARM {
  enum class Architecture {
    ARMv5TE,
    ARMv6K
  };

  ARM(Architecture arch, MemoryBase* memory)
      : cp15(0, memory)
      , arch(arch)
      , memory(memory) {
    BuildConditionTable();
    Reset();
  }

  void Reset();
  void Run(int instructions);

  auto ExceptionBase() -> u32 const { return exception_base; }
  void ExceptionBase(u32 base) { exception_base = base; } 

  void SignalIRQ();

  auto GetPC() -> u32 {
    return state.r15 - (state.cpsr.f.thumb ? 4 : 8);
  }

  void SetPC(u32 value) {
    state.r15 = value;
    if (state.cpsr.f.thumb) {
      ReloadPipeline16();
    } else {
      ReloadPipeline32();
    }
  }

  // TODO: remove this hack.
  bool hit_unimplemented_or_undefined = false;

  typedef void (ARM::*Handler16)(u16);
  typedef void (ARM::*Handler32)(u32);
private:
  friend struct TableGen;
  
  static auto GetRegisterBankByMode(Mode mode) -> Bank;

  void SwitchMode(Mode new_mode);
  void ReloadPipeline16();
  void ReloadPipeline32();
  void BuildConditionTable();
  bool CheckCondition(Condition condition);

  #include "handlers/arithmetic.inl"
  #include "handlers/handler16.inl"
  #include "handlers/handler32.inl"
  #include "handlers/memory.inl"

  bool waiting_for_irq;
  
  // TODO: store exception base configuration in CP15.
  u32 exception_base = 0;

  CP15 cp15;
  Architecture arch;
  State state;
  State::StatusRegister* p_spsr;
  MemoryBase* memory;
  u32 opcode[2];

  bool condition_table[16][16];

  static std::array<Handler16, 2048> s_opcode_lut_16;
  static std::array<Handler32, 8192> s_opcode_lut_32;
};

} // namespace fauxDS::core::arm
