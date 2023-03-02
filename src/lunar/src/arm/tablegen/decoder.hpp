/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunar/integer.hpp>

namespace aura::arm {

// NOTE: we currently can discern all ARM and Thumb instruction types up to ARMv6K,
// except for ARMv6K Hint instructions and most unconditional instructions.

enum class ARMInstrType {
  HalfwordSignedTransfer,
  Multiply,
  SingleDataSwap,
  LoadStoreExclusive,
  StatusTransfer,
  BranchAndExchange,
  BranchAndExchangeJazelle,
  CountLeadingZeros,
  BranchLinkExchange,
  SaturatingAddSubtract,
  Breakpoint,
  SignedHalfwordMultiply,
  DataProcessing,
  Hint,
  SingleDataTransfer,
  Media,
  BlockDataTransfer,
  BranchAndLink,
  CoprocessorLoadStoreAndDoubleRegXfer,
  CoprocessorDataProcessing,
  CoprocessorRegisterXfer,
  SoftwareInterrupt,
  Undefined,
  BranchLinkExchangeImm
};

enum class ThumbInstrType {
  MoveShiftedRegister,
  AddSub,
  MoveCompareAddSubImm,
  ALU,
  HighRegisterOps,
  LoadStoreRelativePC,
  LoadStoreOffsetReg,
  LoadStoreSigned,
  LoadStoreOffsetImm,
  LoadStoreHword,
  LoadStoreRelativeSP,
  LoadAddress,
  AddOffsetToSP,
  SignOrZeroExtend,
  PushPop,
  SetEndianess,
  ChangeProcessorState,
  ReverseBytes,
  SoftwareBreakpoint,
  LoadStoreMultiple,
  ConditionalBranch,
  SoftwareInterrupt,
  UnconditionalBranch,
  LongBranchLinkExchangeSuffix,
  LongBranchLinkPrefix,
  LongBranchLinkSuffix,
  Undefined
};

constexpr auto GetARMInstructionTypeUnconditional(u32 instruction) -> ARMInstrType {
  // TODO: properly decode the unconditional instructions.
  // Missing instructions: PLD, MCR2, MRC2 ...
  if (((instruction >> 25) & 7) == 5) {
    return ARMInstrType::BranchLinkExchangeImm;
  }
  return ARMInstrType::Undefined;
}

constexpr auto GetARMInstructionTypeConditional(u32 instruction) -> ARMInstrType {
  const auto opcode = instruction & 0x0FFFFFFF;
  
  switch (opcode >> 25) {
    case 0b000: {
      // Data processing immediate shift
      // Miscellaneous instructions (A3-4)
      // Data processing register shift
      // Miscellaneous instructions (A3-4)
      // Multiplies (A3-3)
      // Extra load/stores (A3-5)
      const bool set_flags = opcode & (1 << 20);
      const auto opcode2 = (opcode >> 21) & 0xF;
      
      if ((opcode & 0x90) == 0x90) {
        // Multiplies (A3-3)
        // Extra load/stores (A3-5)
        if ((opcode & 0x60) != 0) {
          return ARMInstrType::HalfwordSignedTransfer;
        } else {
          switch ((opcode >> 23) & 3) {
            case 0b00:
            case 0b01:
              return ARMInstrType::Multiply;
            case 0b10:
              return ARMInstrType::SingleDataSwap;
            case 0b11:
              return ARMInstrType::LoadStoreExclusive;
          }
        }
      } else if (!set_flags && opcode2 >= 0b1000 && opcode2 <= 0b1011) {
        // Miscellaneous instructions (A3-4)
        if ((opcode & 0xF0) == 0) {
          return ARMInstrType::StatusTransfer;
        }

        if ((opcode & 0x6000F0) == 0x200010) {
          return ARMInstrType::BranchAndExchange;
        }

        if ((opcode & 0x6000F0) == 0x200020) {
          return ARMInstrType::BranchAndExchangeJazelle;
        }

        if ((opcode & 0x6000F0) == 0x600010) {
          return ARMInstrType::CountLeadingZeros;
        }

        if ((opcode & 0x6000F0) == 0x200030) {
          return ARMInstrType::BranchLinkExchange;
        }

        if ((opcode & 0xF0) == 0x50) {
          return ARMInstrType::SaturatingAddSubtract;
        }

        if ((opcode & 0x6000F0) == 0x200070) {
          return ARMInstrType::Breakpoint;
        }
        
        if ((opcode & 0x90) == 0x80) {
          // Signed halfword multiply (ARMv5 upwards): 
          // SMLAxy, SMLAWy, SMULWy, SMLALxy, SMULxy
          return ARMInstrType::SignedHalfwordMultiply;
        }
      }
      
      // Data processing immediate shift
      // Data processing register shift
      return ARMInstrType::DataProcessing;
    }
    case 0b001: {
      // Data processing immediate
      // Undefined instruction
      // Move immediate to status register
      const bool set_flags = opcode & (1 << 20);

      if (!set_flags) {
        const auto opcode2 = (opcode >> 21) & 0xF;

        switch (opcode2) {
          case 0b1000:
          case 0b1010:
            return ARMInstrType::Undefined;
          case 0b1001:
          case 0b1011:
            return ARMInstrType::StatusTransfer;
        }
      }

      return ARMInstrType::DataProcessing;
    }
    case 0b010: {
      // Load/store immediate offset
      return ARMInstrType::SingleDataTransfer;
    }
    case 0b011: {
      // Load/store register offset
      // Media instructions
      // Architecturally undefined
      if (opcode & 0x10) {
        return ARMInstrType::Media;
      }

      return ARMInstrType::SingleDataTransfer;
    }
    case 0b100: {
      // Load/store multiple
      return ARMInstrType::BlockDataTransfer;
    }
    case 0b101: {
      // Branch and branch with link
      return ARMInstrType::BranchAndLink;
    }
    case 0b110: {
      // Coprocessor load/store and double register transfers
      // TODO: differentiate between load/store and double reg transfer instructions.
      return ARMInstrType::CoprocessorLoadStoreAndDoubleRegXfer;
    }
    case 0b111: {
      // Coprocessor data processing
      // Coprocessor register transfers
      // Software interrupt
      if ((opcode & 0x1000010) == 0) {
        return ARMInstrType::CoprocessorDataProcessing;
      }

      if ((opcode & 0x1000010) == 0x10) {
        return ARMInstrType::CoprocessorRegisterXfer;
      }
      
      return ARMInstrType::SoftwareInterrupt;
    }
  }

  return ARMInstrType::Undefined;
}

constexpr auto GetARMInstructionType(u32 instruction) -> ARMInstrType {
  const auto condition = instruction >> 28;
  if (condition == 15) {
    return GetARMInstructionTypeUnconditional(instruction);
  }
  return GetARMInstructionTypeConditional(instruction);
}

constexpr auto GetThumbInstructionType(u16 instruction) -> ThumbInstrType {
  // TODO: do not use "smaller than" comparisons, since they
  // depend on the order of the if-statements.

  if ((instruction & 0xF800) <  0x1800) return ThumbInstrType::MoveShiftedRegister;
  if ((instruction & 0xF800) == 0x1800) return ThumbInstrType::AddSub;
  if ((instruction & 0xE000) == 0x2000) return ThumbInstrType::MoveCompareAddSubImm;
  if ((instruction & 0xFC00) == 0x4000) return ThumbInstrType::ALU;
  if ((instruction & 0xFC00) == 0x4400) return ThumbInstrType::HighRegisterOps;
  if ((instruction & 0xF800) == 0x4800) return ThumbInstrType::LoadStoreRelativePC;
  if ((instruction & 0xF200) == 0x5000) return ThumbInstrType::LoadStoreOffsetReg;
  if ((instruction & 0xF200) == 0x5200) return ThumbInstrType::LoadStoreSigned;
  if ((instruction & 0xE000) == 0x6000) return ThumbInstrType::LoadStoreOffsetImm;
  if ((instruction & 0xF000) == 0x8000) return ThumbInstrType::LoadStoreHword;
  if ((instruction & 0xF000) == 0x9000) return ThumbInstrType::LoadStoreRelativeSP;
  if ((instruction & 0xF000) == 0xA000) return ThumbInstrType::LoadAddress;
  if ((instruction & 0xFF00) == 0xB000) return ThumbInstrType::AddOffsetToSP;
  if ((instruction & 0xFF00) == 0xB200) return ThumbInstrType::SignOrZeroExtend;
  if ((instruction & 0xF600) == 0xB400) return ThumbInstrType::PushPop;
  if ((instruction & 0xFFE0) == 0xB640) return ThumbInstrType::SetEndianess;
  if ((instruction & 0xFFE0) == 0xB660) return ThumbInstrType::ChangeProcessorState;
  if ((instruction & 0xFF00) == 0xBA00) return ThumbInstrType::ReverseBytes;
  if ((instruction & 0xFF00) == 0xBE00) return ThumbInstrType::SoftwareBreakpoint;
  if ((instruction & 0xF000) == 0xC000) return ThumbInstrType::LoadStoreMultiple;
  if ((instruction & 0xFF00) <  0xDF00) return ThumbInstrType::ConditionalBranch;
  if ((instruction & 0xFF00) == 0xDF00) return ThumbInstrType::SoftwareInterrupt;
  if ((instruction & 0xF800) == 0xE000) return ThumbInstrType::UnconditionalBranch;
  if ((instruction & 0xF800) == 0xE800) return ThumbInstrType::LongBranchLinkExchangeSuffix;
  if ((instruction & 0xF800) == 0xF000) return ThumbInstrType::LongBranchLinkPrefix;
  if ((instruction & 0xF800) == 0xF800) return ThumbInstrType::LongBranchLinkSuffix;

  return ThumbInstrType::Undefined;
}

} // namespace aura::arm
