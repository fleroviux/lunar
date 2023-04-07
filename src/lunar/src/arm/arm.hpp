/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/logger/logger.hpp>
#include <atom/panic.hpp>
#include <array>
#include <lunatic/cpu.hpp>

#include "state.hpp"

namespace lunar::arm {

class ARM final : public lunatic::CPU {
  public:
    // @todo: remove this and use lunatic enumeration.
    enum class Architecture {
      ARMv4T,
      ARMv5TE
    };

    ARM(lunatic::CPU::Descriptor const& descriptor);

    void Reset() override;
    auto IRQLine() -> bool& override;
    auto WaitForIRQ() -> bool& override;
    void ClearICache() override {}
    void ClearICacheRange(u32 address_lo, u32 address_hi) override {}

    auto Run(int cycles) -> int override;

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
    lunatic::Memory* memory;
    std::array<lunatic::Coprocessor*, 16> coprocessors;

    State state;
    State::StatusRegister* p_spsr;
    bool irq_line;

    u32 opcode[2];

    bool condition_table[16][16];

    static std::array<Handler16, 2048> s_opcode_lut_16;
    static std::array<Handler32, 8192> s_opcode_lut_32;
};

} // namespace lunar::arm
