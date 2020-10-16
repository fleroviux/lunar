/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <array>
#include <third_party/fmtlib/include/fmt/format.h>
#include "cp15.hpp"
#include "state.hpp"
#include "../cpu_core.hpp"
#include "../memory.hpp"

namespace fauxDS::core::arm {

class CPUCoreInterpreter final : public CPUCoreBase {
public:
  /**
    * Constructor
    *
    * @param  core    CPU core id
    * @param  arch    ARM architecture revision
    * @param  memory  Memory system
    */
  CPUCoreInterpreter(int core, Architecture arch, MemoryBase* memory)
    : core(core)
    , cp15(core, memory)
    , arch(arch)
    , memory(memory)
  {
    memory->RegisterCore(core, this);
    BuildConditionTable();
    Reset();
  }

  void Reset() override;
  void Run(int instructions) override;

  // TODO(fleroviux): Hold exception raised state,
  // in case that an exception cannot be served immediately?
  // Also implement other exception signals like prefetch/data abort.
  void SignalIRQ() override;

  auto GetPC() -> std::uint32_t override {
    return state.r15 - (state.cpsr.f.thumb ? 4 : 8);
  }

  void SetPC(std::uint32_t value) override {
    state.r15 = value;
    if (state.cpsr.f.thumb) {
      ReloadPipeline16();
    } else {
      ReloadPipeline32();
    }
  }

  bool IsInPrivilegedMode() override {
    // TODO: confirm that the exception modes are privileged.
    return state.cpsr.f.mode != MODE_USR;
  }

  typedef void (CPUCoreInterpreter::*Handler16)(std::uint16_t);
  typedef void (CPUCoreInterpreter::*Handler32)(std::uint32_t);
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

  int core;
  CP15 cp15;
  Architecture arch;
  State state;
  State::StatusRegister* p_spsr;
  MemoryBase* memory;
  std::uint32_t opcode[2];

  bool condition_table[16][16];

  static std::array<Handler16, 2048> s_opcode_lut_16;
  static std::array<Handler32, 8192> s_opcode_lut_32;
};

} // namespace fauxDS::core::arm
