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
      if((int)reg < 8 || reg == GPR::PC || mode == (Mode)state.cpsr.mode) {
        return state.reg[(int)reg];
      }

      if((int)reg < 13 && mode != Mode::FIQ) {
        return state.bank[BANK_NONE][(int)reg - 8];
      }

      return state.bank[GetRegisterBankByMode(mode)][(int)reg - 8];
    }

    PSR GetCPSR() const override {
      return state.cpsr;
    }

    PSR GetSPSR(Mode mode) const override {
      return state.spsr[GetRegisterBankByMode(mode)];
    }

    void SetGPR(GPR reg, u32 value) override {
      state.reg[(int)reg] = value;

      if (reg == GPR::PC) {
        if (state.cpsr.thumb) {
          ReloadPipeline16();
        } else {
          ReloadPipeline32();
        }
      }
    }

    void SetGPR(GPR reg, Mode mode, u32 value) override {
      if((int)reg < 8 || reg == GPR::PC || mode == (Mode)state.cpsr.mode) {
        SetGPR(reg, value);
      } else if((int)reg < 13 && mode != Mode::FIQ) {
        state.bank[BANK_NONE][(int)reg - 8] = value;
      } else {
        state.bank[GetRegisterBankByMode(mode)][(int)reg - 8] = value;
      }
    }

    void SetCPSR(PSR value) override {
      state.cpsr = value;
    }

    void SetSPSR(Mode mode, PSR value) override {
      state.spsr[GetRegisterBankByMode(mode)] = value;
    }

    typedef void (ARM::*Handler16)(u16);
    typedef void (ARM::*Handler32)(u32);

  private:
    enum Condition {
      COND_EQ = 0,
      COND_NE = 1,
      COND_CS = 2,
      COND_CC = 3,
      COND_MI = 4,
      COND_PL = 5,
      COND_VS = 6,
      COND_VC = 7,
      COND_HI = 8,
      COND_LS = 9,
      COND_GE = 10,
      COND_LT = 11,
      COND_GT = 12,
      COND_LE = 13,
      COND_AL = 14,
      COND_NV = 15
    };

    enum Bank {
      BANK_NONE = 0,
      BANK_FIQ  = 1,
      BANK_SVC  = 2,
      BANK_ABT  = 3,
      BANK_IRQ  = 4,
      BANK_UND  = 5
    };

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

    struct State {
      static constexpr int k_bank_count = 6;

      // General Purpose Registers
      union {
        struct {
          u32 r0;
          u32 r1;
          u32 r2;
          u32 r3;
          u32 r4;
          u32 r5;
          u32 r6;
          u32 r7;
          u32 r8;
          u32 r9;
          u32 r10;
          u32 r11;
          u32 r12;
          u32 r13;
          u32 r14;
          u32 r15;
        };
        u32 reg[16];
      };

      // Banked Registers
      u32 bank[k_bank_count][7];

      // Program Status Registers
      aura::arm::CPU::PSR cpsr;
      aura::arm::CPU::PSR spsr[k_bank_count];

      State() {
        for (int i = 0; i < 16; i++) {
          reg[i] = 0;
        }

        for (int b = 0; b < k_bank_count; b++) {
          for (int r = 0; r < 7; r++) {
            bank[b][r] = 0;
          }
          spsr[b] = 0;
        }

        cpsr.word = (uint)aura::arm::CPU::Mode::Supervisor;
        cpsr.mask_irq = 1;
        cpsr.mask_fiq = 1;
      }
    } state;

    PSR* p_spsr;
    bool irq_line;

    u32 opcode[2];

    bool condition_table[16][16];

    static std::array<Handler16, 2048> s_opcode_lut_16;
    static std::array<Handler32, 8192> s_opcode_lut_32;
};

} // namespace lunar::arm
