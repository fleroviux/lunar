/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <array>
#include <lunatic/cpu.hpp>
#include <util/log.hpp>

#include "coprocessor.hpp"
#include "state.hpp"
#include "memory.hpp"

namespace Duality::Core::arm {

struct ARM final : lunatic::CPU {
  // TODO: remove this and use lunatic enumeration.
  enum class Architecture {
    ARMv4T,
    ARMv5TE
  };

  ARM(lunatic::CPU::Descriptor const& descriptor);

  // TODO: make Reset() a virtual member function of lunatic::CPU
  void Reset() override;
  auto IRQLine() -> bool& override;
  void WaitForIRQ() override;
  auto IsWaitingForIRQ() -> bool override;
  void ClearICache() override {}
  void ClearICacheRange(u32 address_lo, u32 address_hi) override {}

  void Run(int cycles) override;

  auto GetGPR(lunatic::GPR reg) const -> u32 override;
  auto GetGPR(lunatic::GPR reg, lunatic::Mode mode) const -> u32 override;
  auto GetCPSR() const -> lunatic::StatusRegister override;
  auto GetSPSR(lunatic::Mode mode) const -> lunatic::StatusRegister override;
  void SetGPR(lunatic::GPR reg, u32 value) override;
  void SetGPR(lunatic::GPR reg, lunatic::Mode mode, u32 value) override;
  void SetCPSR(lunatic::StatusRegister psr) override;
  void SetSPSR(lunatic::Mode mode, lunatic::StatusRegister psr) override;

  typedef void (ARM::*Handler16)(u16);
  typedef void (ARM::*Handler32)(u32);
private:
  friend struct TableGen;
  
  static auto GetRegisterBankByMode(Mode mode) -> Bank;

  void SignalIRQ();
  void ReloadPipeline16();
  void ReloadPipeline32();
  void BuildConditionTable();
  bool CheckCondition(Condition condition);
  void SwitchMode(Mode new_mode);

  #include "handlers/arithmetic.inl"
  #include "handlers/handler16.inl"
  #include "handlers/handler32.inl"
  #include "handlers/memory.inl"

  Architecture arch;
  u32 exception_base = 0;
  bool wait_for_irq = false;
  MemoryBase* memory;
  std::array<Coprocessor*, 16> coprocessors;

  State state;
  State::StatusRegister* p_spsr;
  bool irq_line;

  u32 opcode[2];

  bool condition_table[16][16];
  
  static std::array<Handler16, 2048> s_opcode_lut_16;
  static std::array<Handler32, 8192> s_opcode_lut_32;
};

} // namespace Duality::Core::arm
