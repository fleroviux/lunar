/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

template <std::uint16_t instruction>
static constexpr auto GenerateHandlerThumb() -> Handler16 {
  switch (GetThumbInstructionType(instruction)) {
    case ThumbInstrType::MoveShiftedRegister: {
      const auto opcode  = (instruction >> 11) & 3;
      const auto offset5 = (instruction >>  6) & 0x1F;

      return &CPUCoreInterpreter::Thumb_MoveShiftedRegister<opcode, offset5>;
    }
    case ThumbInstrType::AddSub: {
      const bool immediate = (instruction >> 10) & 1;
      const bool subtract  = (instruction >>  9) & 1;
      const auto field3 = (instruction >> 6) & 7;

      return &CPUCoreInterpreter::Thumb_AddSub<immediate, subtract, field3>;
    }
    case ThumbInstrType::MoveCompareAddSubImm: {
      const auto opcode = (instruction >> 11) & 3;
      const auto rD = (instruction >> 8) & 7;

      return &CPUCoreInterpreter::Thumb_Op3<opcode, rD>;
    }
    case ThumbInstrType::ALU: {
      const auto opcode = (instruction >> 6) & 0xF;

      return &CPUCoreInterpreter::Thumb_ALU<opcode>;
    }
    case ThumbInstrType::HighRegisterOps: {
      const auto opcode = (instruction >> 8) & 3;
      const bool high1  = (instruction >> 7) & 1;
      const bool high2  = (instruction >> 6) & 1;

      return &CPUCoreInterpreter::Thumb_HighRegisterOps_BX<opcode, high1, high2>;
    }
    case ThumbInstrType::LoadStoreRelativePC: {
      const auto rD = (instruction >> 8) & 7;

      return &CPUCoreInterpreter::Thumb_LoadStoreRelativePC<rD>;
    }
    case ThumbInstrType::LoadStoreOffsetReg: {
      const auto opcode = (instruction >> 10) & 3;
      const auto rO = (instruction >>  6) & 7;

      return &CPUCoreInterpreter::Thumb_LoadStoreOffsetReg<opcode, rO>;
    }
    case ThumbInstrType::LoadStoreSigned: {
      const auto opcode = (instruction >> 10) & 3;
      const auto rO = (instruction >>  6) & 7;

      return &CPUCoreInterpreter::Thumb_LoadStoreSigned<opcode, rO>;
    }
    case ThumbInstrType::LoadStoreOffsetImm: {
      const auto opcode  = (instruction >> 11) & 3;
      const auto offset5 = (instruction >>  6) & 0x1F;

      return &CPUCoreInterpreter::Thumb_LoadStoreOffsetImm<opcode, offset5>;
    }
    case ThumbInstrType::LoadStoreHword: {
      const bool load = (instruction >> 11) & 1;
      const auto offset5 = (instruction >> 6) & 0x1F;

      return &CPUCoreInterpreter::Thumb_LoadStoreHword<load, offset5>;
    }
    case ThumbInstrType::LoadStoreRelativeSP: {
      const bool load = (instruction >> 11) & 1;
      const auto rD = (instruction >> 8) & 7;

      return &CPUCoreInterpreter::Thumb_LoadStoreRelativeToSP<load, rD>;
    }
    case ThumbInstrType::LoadAddress: {
      const bool use_r13 = (instruction >> 11) & 1;
      const auto rD = (instruction >> 8) & 7;

      return &CPUCoreInterpreter::Thumb_LoadAddress<use_r13, rD>;
    }
    case ThumbInstrType::AddOffsetToSP: {
      const bool subtract = (instruction >> 7) & 1;

      return &CPUCoreInterpreter::Thumb_AddOffsetToSP<subtract>;
    }
    case ThumbInstrType::SignOrZeroExtend: {
      const auto opcode = (instruction >> 6) & 3;

      return &CPUCoreInterpreter::Thumb_SignOrZeroExtend<opcode>;
    }
    case ThumbInstrType::PushPop: {
      const bool load  = (instruction >> 11) & 1;
      const bool pc_lr = (instruction >>  8) & 1;

      return &CPUCoreInterpreter::Thumb_PushPop<load, pc_lr>;
    }
    case ThumbInstrType::ReverseBytes: {
      return &CPUCoreInterpreter::Thumb_ReverseBytes;
    }
    case ThumbInstrType::LoadStoreMultiple: {
      const bool load = (instruction >> 11) & 1;
      const auto rB = (instruction >> 8) & 7;

      return &CPUCoreInterpreter::Thumb_LoadStoreMultiple<load, rB>;
    }
    case ThumbInstrType::ConditionalBranch: {
      const auto condition = (instruction >> 8) & 0xF;

      return &CPUCoreInterpreter::Thumb_ConditionalBranch<condition>;
    }
    case ThumbInstrType::SoftwareInterrupt: {
      return &CPUCoreInterpreter::Thumb_SWI;
    }
    case ThumbInstrType::UnconditionalBranch: {
      return &CPUCoreInterpreter::Thumb_UnconditionalBranch;
    }
    case ThumbInstrType::LongBranchLinkPrefix: {
      return &CPUCoreInterpreter::Thumb_LongBranchLinkPrefix;
    }
    case ThumbInstrType::LongBranchLinkSuffix: {
      return &CPUCoreInterpreter::Thumb_LongBranchLinkSuffix<false>;
    }
    case ThumbInstrType::LongBranchLinkExchangeSuffix: {
      return &CPUCoreInterpreter::Thumb_LongBranchLinkSuffix<true>;
    }
  }

  return &CPUCoreInterpreter::Thumb_Unimplemented;
}
