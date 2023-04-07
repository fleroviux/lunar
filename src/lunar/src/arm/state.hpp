/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/integer.hpp>

namespace lunar::arm {

enum Mode : unsigned int {
  MODE_USR = 0x10,
  MODE_FIQ = 0x11,
  MODE_IRQ = 0x12,
  MODE_SVC = 0x13,
  MODE_ABT = 0x17,
  MODE_UND = 0x1B,
  MODE_SYS = 0x1F
};
  
enum Bank {
  BANK_NONE = 0,
  BANK_FIQ  = 1,
  BANK_SVC  = 2,
  BANK_ABT  = 3,
  BANK_IRQ  = 4,
  BANK_UND  = 5
};

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

enum BankedRegister {
  BANK_R8  = 0,
  BANK_R9  = 1,
  BANK_R10 = 2,
  BANK_R11 = 3,
  BANK_R12 = 4,
  BANK_R13 = 5,
  BANK_R14 = 6
};

class State {
  public:
    static constexpr int k_bank_count = 6;

    union StatusRegister {
      struct {
        Mode mode : 5;
        unsigned int thumb : 1;
        unsigned int mask_fiq : 1;
        unsigned int mask_irq : 1;
        unsigned int reserved : 19;
        unsigned int q : 1;
        unsigned int v : 1;
        unsigned int c : 1;
        unsigned int z : 1;
        unsigned int n : 1;
      } f;
      u32 v;
    };

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
    StatusRegister cpsr;
    StatusRegister spsr[k_bank_count];

    State() { Reset(); }

    void Reset() {
      for (int i = 0; i < 16; i++) {
        reg[i] = 0;
      }

      for (int b = 0; b < k_bank_count; b++) {
        for (int r = 0; r < 7; r++) {
          bank[b][r] = 0;
        }
        spsr[b].v = 0;
      }

      cpsr.v = MODE_SVC;
      cpsr.f.mask_irq = 1;
      cpsr.f.mask_fiq = 1;
    }
};

} // namespace lunar::arm
