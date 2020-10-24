/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

enum class ThumbDataOp {
  AND = 0,
  EOR = 1,
  LSL = 2,
  LSR = 3,
  ASR = 4,
  ADC = 5,
  SBC = 6,
  ROR = 7,
  TST = 8,
  NEG = 9,
  CMP = 10,
  CMN = 11,
  ORR = 12,
  MUL = 13,
  BIC = 14,
  MVN = 15
};

template <int op, int imm>
void Thumb_MoveShiftedRegister(u16 instruction) {
  // THUMB.1 Move shifted register
  int dst   = (instruction >> 0) & 7;
  int src   = (instruction >> 3) & 7;
  int carry = state.cpsr.f.c;

  u32 result = state.reg[src];

  DoShift(op, result, imm, carry, true);

  /* Update flags */
  state.cpsr.f.c = carry;
  state.cpsr.f.z = (result == 0);
  state.cpsr.f.n = result >> 31;

  state.reg[dst] = result;
  state.r15 += 2;
}

template <bool immediate, bool subtract, int field3>
void Thumb_AddSub(u16 instruction) {
  int dst = (instruction >> 0) & 7;
  int src = (instruction >> 3) & 7;
  u32 operand = immediate ? field3 : state.reg[field3];

  if (subtract) {
    state.reg[dst] = SUB(state.reg[src], operand, true);
  } else {
    state.reg[dst] = ADD(state.reg[src], operand, true);
  }

  state.r15 += 2;
}

template <int op, int dst>
void Thumb_Op3(u16 instruction) {
  // THUMB.3 Move/compare/add/subtract immediate
  u32 imm = instruction & 0xFF;

  switch (op) {
  case 0b00:
    /* MOV rD, #imm */
    state.reg[dst] = imm;
    state.cpsr.f.n = 0;
    state.cpsr.f.z = imm == 0;
    break;
  case 0b01:
    /* CMP rD, #imm */
    SUB(state.reg[dst], imm, true);
    break;
  case 0b10:
    /* ADD rD, #imm */
    state.reg[dst] = ADD(state.reg[dst], imm, true);
    break;
  case 0b11:
    /* SUB rD, #imm */
    state.reg[dst] = SUB(state.reg[dst], imm, true);
    break;
  }

  state.r15 += 2;
}

template <int op>
void Thumb_ALU(u16 instruction) {
  int dst = (instruction >> 0) & 7;
  int src = (instruction >> 3) & 7;

  /* NOTE: Order of opcodes was rearranged. */
  switch (static_cast<ThumbDataOp>(op)) {
  case ThumbDataOp::MVN:
    state.reg[dst] = ~state.reg[src];
    SetZeroAndSignFlag(state.reg[dst]);
    break;

  /* Bitwise logical */
  case ThumbDataOp::AND:
    state.reg[dst] &= state.reg[src];
    SetZeroAndSignFlag(state.reg[dst]);
    break;
  case ThumbDataOp::BIC:
    state.reg[dst] &= ~state.reg[src];
    SetZeroAndSignFlag(state.reg[dst]);
    break;
  case ThumbDataOp::ORR:
    state.reg[dst] |= state.reg[src];
    SetZeroAndSignFlag(state.reg[dst]);
    break;
  case ThumbDataOp::EOR:
    state.reg[dst] ^= state.reg[src];
    SetZeroAndSignFlag(state.reg[dst]);
    break;
  case ThumbDataOp::TST: {
    u32 result = state.reg[dst] & state.reg[src];
    SetZeroAndSignFlag(result);
    break;
  }

  /* LSL, LSR, ASR, ROR */
  case ThumbDataOp::LSL: {
    int carry = state.cpsr.f.c;
    LSL(state.reg[dst], state.reg[src], carry);
    SetZeroAndSignFlag(state.reg[dst]);
    state.cpsr.f.c = carry;
    break;
  }
  case ThumbDataOp::LSR: {
    int carry = state.cpsr.f.c;
    LSR(state.reg[dst], state.reg[src], carry, false);
    SetZeroAndSignFlag(state.reg[dst]);
    state.cpsr.f.c = carry;
    break;
  }
  case ThumbDataOp::ASR: {
    int carry = state.cpsr.f.c;
    ASR(state.reg[dst], state.reg[src], carry, false);
    SetZeroAndSignFlag(state.reg[dst]);
    state.cpsr.f.c = carry;
    break;
  }
  case ThumbDataOp::ROR: {
    int carry = state.cpsr.f.c;
    ROR(state.reg[dst], state.reg[src], carry, false);
    SetZeroAndSignFlag(state.reg[dst]);
    state.cpsr.f.c = carry;
    break;
  }

  /* Add/Sub with carry, NEG, comparison, multiply */
  case ThumbDataOp::ADC:
    state.reg[dst] = ADC(state.reg[dst], state.reg[src], true);
    break;
  case ThumbDataOp::SBC:
    state.reg[dst] = SBC(state.reg[dst], state.reg[src], true);
    break;
  case ThumbDataOp::NEG:
    state.reg[dst] = SUB(0, state.reg[src], true);
    break;
  case ThumbDataOp::CMP:
    SUB(state.reg[dst], state.reg[src], true);
    break;
  case ThumbDataOp::CMN:
    ADD(state.reg[dst], state.reg[src], true);
    break;
  case ThumbDataOp::MUL:
    state.reg[dst] *= state.reg[src];
    SetZeroAndSignFlag(state.reg[dst]);
    state.cpsr.f.c = 0;
    break;
  }

  state.r15 += 2;
}

template <int op, bool high1, bool high2>
void Thumb_HighRegisterOps_BX(u16 instruction) {
  // THUMB.5 Hi register operations/branch exchange
  int dst = (instruction >> 0) & 7;
  int src = (instruction >> 3) & 7;

  // Instruction may access higher registers r8 - r15 ("Hi register").
  // This is archieved using two extra bits that displace the register number by 8.
  if (high1) dst |= 8;
  if (high2) src |= 8;

  u32 operand = state.reg[src];

  if (src == 15) operand &= ~1;

  /* Check for Branch & Exchange (bx) instruction. */
  if (op == 3) {
    /* NOTE: "high1" actually is the link bit for "bx" instructions. */
    if (high1) {
      state.r14 = (state.r15 - 2) | 1;
    }

    /* LSB indicates new instruction set (0 = ARM, 1 = THUMB) */
    if (operand & 1) {
      state.r15 = operand & ~1;
      ReloadPipeline16();
    } else {
      state.cpsr.f.thumb = 0;
      state.r15 = operand & ~3;
      ReloadPipeline32();
    }
  /* Check for Compare (CMP) instruction. */
  } else if (op == 1) {
    SUB(state.reg[dst], operand, true);
    state.r15 += 2;
  /* Otherwise instruction is ADD or MOV. */
  } else {
    if (op == 0) state.reg[dst] += operand;
    if (op == 2) state.reg[dst]  = operand;

    if (dst == 15) {
      state.r15 &= ~1;
      ReloadPipeline16();
    } else {
      state.r15 += 2;
    }
  }
}

template <int dst>
void Thumb_LoadStoreRelativePC(u16 instruction) {
  u32 offset  = instruction & 0xFF;
  u32 address = (state.r15 & ~2) + (offset << 2);

  state.reg[dst] = ReadWord(address);
  state.r15 += 2;
}

template <int op, int off>
void Thumb_LoadStoreOffsetReg(u16 instruction) {
  int dst  = (instruction >> 0) & 7;
  int base = (instruction >> 3) & 7;

  u32 address = state.reg[base] + state.reg[off];

  switch (op) {
  case 0b00: // STR
    WriteWord(address, state.reg[dst]);
    break;
  case 0b01: // STRB
    WriteByte(address, (u8)state.reg[dst]);
    break;
  case 0b10: // LDR
    state.reg[dst] = ReadWordRotate(address);
    break;
  case 0b11: // LDRB
    state.reg[dst] = ReadByte(address);
    break;
  }

  state.r15 += 2;
}

template <int op, int off>
void Thumb_LoadStoreSigned(u16 instruction) {
  int dst  = (instruction >> 0) & 7;
  int base = (instruction >> 3) & 7;

  u32 address = state.reg[base] + state.reg[off];

  switch (op) {
  case 0b00:
    /* STRH rD, [rB, rO] */
    WriteHalf(address, state.reg[dst]);
    break;
  case 0b01:
    /* LDSB rD, [rB, rO] */
    state.reg[dst] = ReadByteSigned(address);
    break;
  case 0b10:
    /* LDRH rD, [rB, rO] */
    state.reg[dst] = ReadHalfRotate(address);
    break;
  case 0b11:
    /* LDSH rD, [rB, rO] */
    state.reg[dst] = ReadHalfSigned(address);
    break;
  }

  state.r15 += 2;
}

template <int op, int imm>
void Thumb_LoadStoreOffsetImm(u16 instruction) {
  int dst  = (instruction >> 0) & 7;
  int base = (instruction >> 3) & 7;

  switch (op) {
  case 0b00:
    /* STR rD, [rB, #imm] */
    WriteWord(state.reg[base] + imm * 4, state.reg[dst]);
    break;
  case 0b01:
    /* LDR rD, [rB, #imm] */
    state.reg[dst] = ReadWordRotate(state.reg[base] + imm * 4);
    break;
  case 0b10:
    /* STRB rD, [rB, #imm] */
    WriteByte(state.reg[base] + imm, state.reg[dst]);
    break;
  case 0b11:
    /* LDRB rD, [rB, #imm] */
    state.reg[dst] = ReadByte(state.reg[base] + imm);
    break;
  }

  state.r15 += 2;
}

template <bool load, int imm>
void Thumb_LoadStoreHword(u16 instruction) {
  int dst  = (instruction >> 0) & 7;
  int base = (instruction >> 3) & 7;

  u32 address = state.reg[base] + imm * 2;

  if (load) {
    state.reg[dst] = ReadHalfRotate(address);
  } else {
    WriteHalf(address, state.reg[dst]);
  }

  state.r15 += 2;
}

template <bool load, int dst>
void Thumb_LoadStoreRelativeToSP(u16 instruction) {
  u32 offset  = instruction & 0xFF;
  u32 address = state.reg[13] + offset * 4;

  if (load) {
    state.reg[dst] = ReadWordRotate(address);
  } else {
    WriteWord(address, state.reg[dst]);
  }

  state.r15 += 2;
}

template <bool stackptr, int dst>
void Thumb_LoadAddress(u16 instruction) {
  u32 offset = (instruction  & 0xFF) << 2;

  if (stackptr) {
    state.reg[dst] = state.r13 + offset;
  } else {
    state.reg[dst] = (state.r15 & ~2) + offset;
  }

  state.r15 += 2;
}

template <bool sub>
void Thumb_AddOffsetToSP(u16 instruction) {
  u32 offset = (instruction  & 0x7F) * 4;

  state.r13 += sub ? -offset : offset;
  state.r15 += 2;
}

template <bool pop, bool rbit>
void Thumb_PushPop(u16 instruction) {
  u32 address = state.r13;

  if (pop) {
    for (int reg = 0; reg <= 7; reg++) {
      if (instruction & (1<<reg)) {
        state.reg[reg] = ReadWord(address);
        address += 4;
      }
    }

    if (rbit) {
      state.reg[15] = ReadWord(address);
      state.reg[13] = address + 4;
      if (state.r15 & 1) {
        state.r15 &= ~1;
        ReloadPipeline16();
      } else {
        state.cpsr.f.thumb = 0;
        ReloadPipeline32();
      }
      return;
    }

    state.r13 = address;
  } else {
    /* Calculate internal start address (final r13 value) */
    for (int reg = 0; reg <= 7; reg++) {
      if (instruction & (1 << reg))
        address -= 4;
    }
    if (rbit) address -= 4;

    /* Store address in r13 before we mess with it. */
    state.r13 = address;

    for (int reg = 0; reg <= 7; reg++) {
      if (instruction & (1<<reg)) {
        WriteWord(address, state.reg[reg]);
        address += 4;
      }
    }

    if (rbit) {
      WriteWord(address, state.r14);
    }
  }

  state.r15 += 2;
}

void Thumb_ReverseBytes(u16 instruction) {
  int dst = (instruction >> 0) & 7;
  int src = (instruction >> 3) & 7;
  int opcode = (instruction >> 6) & 3;

  u32 result = state.reg[src];

  // TODO: could make 'opcode' a template parameter.
  // Also verify that these instructions work the way they're supposed to.
  switch (opcode) {
  case 0: {
    // REV
    result = (result << 16) | (result >> 16);
    result = ((result & 0xFF00FF00) >> 8) | ((result & 0x00FF00FF) << 8);
    break;
  }
  case 1: {
    // REV16
    result = ((result & 0xFF00FF00) >> 8) | ((result & 0x00FF00FF) << 8);
    break;
  }
  case 3: {
    // REVSH
    result = ((result & 0xFF00) >> 8) | ((result & 0x00FF) << 8);
    if (result & 0x80) {
      result |= 0xFFFF0000;
    }
    break;
  }
  default: {
    Thumb_Unimplemented(instruction);
    return;
  }
  }

  state.reg[dst] = result;
  state.r15 += 2;
}

template <bool load, int base>
void Thumb_LoadStoreMultiple(u16 instruction) {
  u32 address = state.reg[base];

  // TODO: I don't know if this behaves the same way in ARMv5TE and ARMv6K.
  // Eventhough ARM LDM/STM on ARMv6K seem to roughly work like this,
  // LDM/STM edge-cases historically have been different for ARM and Thumb.
  // It is also unclear whether Thumb STM should be affected by the same
  // edge-case like ARM11's ARM STM, where storing the base register
  // will store the final base address, unless it is the first or second register.
  if (load) {
    for (int i = 0; i <= 7; i++) {
      if (instruction & (1<<i)) {
        state.reg[i] = ReadWord(address);
        address += 4;
      }
    }
    
    if (~instruction & (1<<base)) {
      state.reg[base] = address;
    }
  } else {
    for (int reg = 0; reg <= 7; reg++) {
      if (instruction & (1<<reg)) {
        WriteWord(address, state.reg[reg]);
        address += 4;
      }
    }

    state.reg[base] = address;
  }

  state.r15 += 2;
}

template <int cond>
void Thumb_ConditionalBranch(u16 instruction) {
  if (CheckCondition(static_cast<Condition>(cond))) {
    u32 imm = instruction & 0xFF;

    /* Sign-extend immediate value. */
    if (imm & 0x80) {
      imm |= 0xFFFFFF00;
    }

    state.r15 += imm * 2;
    ReloadPipeline16();
  } else {
    state.r15 += 2;
  }
}

void Thumb_SWI(u16 instruction) {
  /* Save return address and program status. */
  state.bank[BANK_SVC][BANK_R14] = state.r15 - 2;
  state.spsr[BANK_SVC].v = state.cpsr.v;

  /* Switch to SVC mode and disable interrupts. */
  SwitchMode(MODE_SVC);
  state.cpsr.f.thumb = 0;
  state.cpsr.f.mask_irq = 1;

  /* Jump to execution vector */
  state.r15 = ExceptionBase() + 0x08;
  ReloadPipeline32();
}

void Thumb_UnconditionalBranch(u16 instruction) {
  u32 imm = (instruction & 0x3FF) * 2;

  /* Sign-extend immediate value. */
  if (instruction & 0x400) {
    imm |= 0xFFFFF800;
  }

  state.r15 += imm;
  ReloadPipeline16();
}

void Thumb_LongBranchLinkPrefix(u16 instruction) {
  u32 imm = (instruction & 0x7FF) << 12;

  if (imm & 0x400000) {
    imm |= 0xFF800000;
  }

  state.r14 = state.r15 + imm;
  state.r15 += 2;
}

template <bool exchange>
void Thumb_LongBranchLinkSuffix(u16 instruction) {
  u32 imm  = instruction & 0x7FF;
  u32 temp = state.r15 - 2;

  state.r15 = (state.r14 & ~1) + imm * 2;
  state.r14 = temp | 1;
  if (exchange) {
    state.r15 &= ~3;
    state.cpsr.f.thumb = 0;
    ReloadPipeline32();
  } else {
    ReloadPipeline16();
  }
}

template <int opcode>
void Thumb_SignOrZeroExtend(u16 instruction) {
  int dst =  instruction & 7;
  int src = (instruction >> 3) & 7;
  
  switch (opcode) {
    case 0b00: {
      /* SXTH - signed extend half word */
      state.reg[dst] = state.reg[src] & 0xFFFF;
      if (state.reg[dst] & 0x8000) {
        state.reg[dst] |= 0xFFFF0000;
      }
      break;
    }
    case 0b01: {
      /* SXTB - signed extend byte */
      state.reg[dst] = state.reg[src] & 0xFF;
      if (state.reg[dst] & 0x80) {
        state.reg[dst] |= 0xFFFFFF00;
      }
      break;
    }
    case 0b10: {
      /* UXTH - unsigned extend half word */
      state.reg[dst] = state.reg[src] & 0xFFFF;
      break;
    }
    case 0b11: {
      /* UXTB - unsigned extend byte */
      state.reg[dst] = state.reg[src] & 0xFF;
      break;
    }
  }

  state.r15 += 2;
}

void Thumb_Unimplemented(u16 instruction) {
  LOG_ERROR("Unimplemented Thumb instruction: 0x{0:04X} @ r15 = 0x{1:08X}", instruction, state.r15);
  hit_unimplemented_or_undefined = true;
  state.r15 += 2;
}
