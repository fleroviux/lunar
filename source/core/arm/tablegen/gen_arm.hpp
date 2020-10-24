/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

enum class MultiplyOpcode {
  MUL   = 0b000,
  MLA   = 0b001,
  UMAAL = 0b010,
  UMULL = 0b100,
  UMLAL = 0b101,
  SMULL = 0b110,
  SMLAL = 0b111
};

enum class SignedMultiplyOpcode {
  SMLAxy  = 0b1000,
  SM__Wy  = 0b1001,
  SMLALxy = 0b1010,
  SMULxy  = 0b1011
};

template <std::uint32_t instruction>
static constexpr auto GenerateHandlerARM() -> Handler32 {
  const bool pre  = instruction & (1 << 24);
  const bool add  = instruction & (1 << 23);
  const bool wb   = instruction & (1 << 21);
  const bool load = instruction & (1 << 20);
  
  switch (GetARMInstructionType(instruction)) {
    case ARMInstrType::HalfwordSignedTransfer: {
      const bool immediate = instruction & (1 << 22);
      const auto opcode = (instruction >> 5) & 3;
      
      return &ARM::ARM_SingleHalfwordDoubleTransfer<pre, add, immediate, wb, load, opcode>;
    }
    case ARMInstrType::Multiply: {
      const bool set_flags = instruction & (1 << 20);

      switch (static_cast<MultiplyOpcode>((instruction >> 21) & 0xF)) {
        case MultiplyOpcode::MUL: {
          return &ARM::ARM_Multiply<false, set_flags>;
        }
        case MultiplyOpcode::MLA: {
          return &ARM::ARM_Multiply<true, set_flags>;
        }
        case MultiplyOpcode::UMAAL: {
          return &ARM::ARM_UnsignedMultiplyLongAccumulateAccumulate;
        }
        case MultiplyOpcode::UMULL: {
          return &ARM::ARM_MultiplyLong<false, false, set_flags>;
        }
        case MultiplyOpcode::UMLAL: {
          return &ARM::ARM_MultiplyLong<false, true, set_flags>;
        }
        case MultiplyOpcode::SMULL: {
          return &ARM::ARM_MultiplyLong<true, false, set_flags>;
        }
        case MultiplyOpcode::SMLAL: {
          return &ARM::ARM_MultiplyLong<true, true, set_flags>;
        }
      }
      
      break;
    }
    case ARMInstrType::SingleDataSwap: {
      const bool byte = instruction & (1 << 22);
      
      return &ARM::ARM_SingleDataSwap<byte>;
    }
    case ARMInstrType::LoadStoreExclusive: {
      return &ARM::ARM_Unimplemented;
    }
    case ARMInstrType::StatusTransfer: {
      const bool immediate = instruction & (1 << 25);
      const bool use_spsr  = instruction & (1 << 22);
      const bool to_status = instruction & (1 << 21);

      return &ARM::ARM_StatusTransfer<immediate, use_spsr, to_status>;
    }
    case ARMInstrType::BranchAndExchange: {
      return &ARM::ARM_BranchAndExchangeMaybeLink<false>;
    }
    case ARMInstrType::BranchAndExchangeJazelle: {
      return &ARM::ARM_Unimplemented;
    }
    case ARMInstrType::CountLeadingZeros: {
      return &ARM::ARM_CountLeadingZeros;
    }
    case ARMInstrType::BranchLinkExchange: {
      return &ARM::ARM_BranchAndExchangeMaybeLink<true>;
    }
    case ARMInstrType::SaturatingAddSubtract: {
      const int opcode = (instruction >> 20) & 0xF;
      
      return &ARM::ARM_SaturatingAddSubtract<opcode>;
    }
    case ARMInstrType::Breakpoint: {
      return &ARM::ARM_Unimplemented;
    }
    case ARMInstrType::SignedMultiplies: {
      const bool x = instruction & (1 << 5);
      const bool y = instruction & (1 << 6);
  
      switch (static_cast<SignedMultiplyOpcode>((instruction >> 21) & 0xF)) {
        case SignedMultiplyOpcode::SMLAxy: {
          return &ARM::ARM_SignedHalfwordMultiply<true, x, y>;
        }
        case SignedMultiplyOpcode::SM__Wy: {
          /* NOTE: "x" in this case encodes "NOT accumulate". */
          return &ARM::ARM_SignedWordHalfwordMultiply<!x, y>;
        }
        case SignedMultiplyOpcode::SMLALxy: {
          return &ARM::ARM_SignedHalfwordMultiplyLongAccumulate<x, y>;
        }
        case SignedMultiplyOpcode::SMULxy: {
          return &ARM::ARM_SignedHalfwordMultiply<false, x, y>;
        }
      }
      
      break;
    }
    case ARMInstrType::DataProcessing: {
      const bool immediate = instruction & (1 << 25);
      const bool set_flags = instruction & (1 << 20);
      const auto opcode = (instruction >> 21) & 0xF;
      const auto field4 = (instruction >>  4) & 0xF;

      return &ARM::ARM_DataProcessing<immediate, opcode, set_flags, field4>;
    }
    case ARMInstrType::Hint: {
      // We cannot discern "hint" instructions from "status transfer" 
      // instructions, as the relevant bits are not included in the index
      // to the opcode table. Doubling the table's size for one instruction
      // seems a bit unreasonable. Therefore the hint instructions are handled 
      // separately in the "ARM_StatusTransfer" method.
      break;
    }
    case ARMInstrType::SingleDataTransfer: {
      const bool immediate = ~instruction & (1 << 25);
      const bool byte = instruction & (1 << 22);
      
      return &ARM::ARM_SingleDataTransfer<immediate, pre, add, byte, wb, load>;
    }
    case ARMInstrType::Media: {
      return &ARM::ARM_Unimplemented;
    }
    case ARMInstrType::BlockDataTransfer: {
      const bool user_mode = instruction & (1 << 22);
            
      return &ARM::ARM_BlockDataTransfer<pre, add, user_mode, wb, load>;
    }
    case ARMInstrType::BranchAndLink: {
      return &ARM::ARM_BranchAndLink<(instruction >> 24) & 1>;
    }
    case ARMInstrType::CoprocessorLoadStoreAndDoubleRegXfer:
    case ARMInstrType::CoprocessorDataProcessing: {
      return &ARM::ARM_Unimplemented;
    }
    case ARMInstrType::CoprocessorRegisterXfer: {
      return &ARM::ARM_CoprocessorRegisterTransfer;
    }
    case ARMInstrType::SoftwareInterrupt: {
      return &ARM::ARM_SWI;
    }
    case ARMInstrType::BranchLinkExchangeImm: {
      return &ARM::ARM_BranchLinkExchangeImm;
    }
    case ARMInstrType::Unconditional: { // placeholder
      return &ARM::ARM_Unimplemented;
    }
  }

  return &ARM::ARM_Undefined;
}
