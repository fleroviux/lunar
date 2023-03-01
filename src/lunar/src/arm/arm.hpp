/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <array>
#include <aura/arm/cpu.hpp>
#include <aura/arm/memory.hpp>
#include <lunatic/cpu.hpp>
#include <lunar/log.hpp>

#include "state.hpp"

namespace lunar::arm {

class ARM final : public aura::arm::CPU {
  public:
    // @todo: enumerate processors instead of architectures
    enum class Architecture {
      ARMv4T,
      ARMv5TE
    };

    explicit ARM(lunatic::CPU::Descriptor const& descriptor);

    void Reset() override;

    u32 GetExceptionBase() const override {
      return exception_base;
    }

    void SetExceptionBase(u32 address) override {
      exception_base = address;
    }

    bool GetWaitingForIRQ() const override {
      return wait_for_irq;
    }

    void SetWaitingForIRQ(bool value) override {
      wait_for_irq = value;
    }

    bool GetIRQFlag() const override {
      return irq_line;
    }

    void SetIRQFlag(bool value) override {
      irq_line = value;
    }

    void Run(int cycles) override;

    u32 GetGPR(GPR reg) const override {
      return state.reg[(int)reg];
    }

    u32 GetGPR(GPR reg, Mode mode) const override {
      // @todo: do not cast mode
      if((int)reg < 8 || reg == GPR::PC || (uint)mode == state.cpsr.f.mode) {
        return state.reg[(int)reg];
      }

      if((int)reg < 13 && mode != Mode::FIQ) {
        return state.bank[BANK_NONE][(int)reg - 8];
      }

      return state.bank[GetRegisterBankByMode(mode)][(int)reg - 8];
    }

    PSR GetCPSR() const override {
      return state.cpsr.v;
    }

    PSR GetSPSR(Mode mode) const override {
      return state.spsr[GetRegisterBankByMode(mode)].v;
    }

    void SetGPR(GPR reg, u32 value) override {
      state.reg[(int)reg] = value;

      if (reg == GPR::PC) {
        if (state.cpsr.f.thumb) {
          ReloadPipeline16();
        } else {
          ReloadPipeline32();
        }
      }
    }

    void SetGPR(GPR reg, Mode mode, u32 value) override {
      // @todo: do not cast mode
      if((int)reg < 8 || reg == GPR::PC || (uint)mode == state.cpsr.f.mode) {
        SetGPR(reg, value);
      } else if((int)reg < 13 && mode != Mode::FIQ) {
        state.bank[BANK_NONE][(int)reg - 8] = value;
      } else {
        state.bank[GetRegisterBankByMode(mode)][(int)reg - 8] = value;
      }
    }

    void SetCPSR(PSR value) override {
      state.cpsr.v = value.word;
    }

    void SetSPSR(Mode mode, PSR value) override {
      state.spsr[GetRegisterBankByMode(mode)].v = value.word;
    }

    // TODO: remove these!! they only exist for compatibility!
    bool& IRQLine() { return irq_line; }
    bool& WaitForIRQ() { return wait_for_irq; }

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
