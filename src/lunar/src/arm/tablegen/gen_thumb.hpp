
// Copyright (C) 2022 fleroviux

template <u16 instruction>
static constexpr auto GenerateHandlerThumb() -> Handler16 {
  switch (GetThumbInstructionType(instruction)) {
    case ThumbInstrType::MoveShiftedRegister: {
      const auto opcode  = (instruction >> 11) & 3;
      const auto offset5 = (instruction >>  6) & 0x1F;

      return &ARM::Thumb_MoveShiftedRegister<opcode, offset5>;
    }
    case ThumbInstrType::AddSub: {
      const bool immediate = (instruction >> 10) & 1;
      const bool subtract  = (instruction >>  9) & 1;
      const auto field3 = (instruction >> 6) & 7;

      return &ARM::Thumb_AddSub<immediate, subtract, field3>;
    }
    case ThumbInstrType::MoveCompareAddSubImm: {
      const auto opcode = (instruction >> 11) & 3;
      const auto rD = (instruction >> 8) & 7;

      return &ARM::Thumb_MoveCompareAddSubImm<opcode, rD>;
    }
    case ThumbInstrType::ALU: {
      const auto opcode = static_cast<ARM::ThumbDataOp>((instruction >> 6) & 0xF);

      return &ARM::Thumb_ALU<opcode>;
    }
    case ThumbInstrType::HighRegisterOps: {
      const auto opcode = static_cast<ARM::ThumbHighRegOp>((instruction >> 8) & 3);
      const bool high1 = (instruction >> 7) & 1;
      const bool high2 = (instruction >> 6) & 1;

      return &ARM::Thumb_HighRegisterOps_BX<opcode, high1, high2>;
    }
    case ThumbInstrType::LoadStoreRelativePC: {
      const auto rD = (instruction >> 8) & 7;

      return &ARM::Thumb_LoadStoreRelativePC<rD>;
    }
    case ThumbInstrType::LoadStoreOffsetReg: {
      const auto opcode = (instruction >> 10) & 3;
      const auto rO = (instruction >>  6) & 7;

      return &ARM::Thumb_LoadStoreOffsetReg<opcode, rO>;
    }
    case ThumbInstrType::LoadStoreSigned: {
      const auto opcode = (instruction >> 10) & 3;
      const auto rO = (instruction >>  6) & 7;

      return &ARM::Thumb_LoadStoreSigned<opcode, rO>;
    }
    case ThumbInstrType::LoadStoreOffsetImm: {
      const auto opcode  = (instruction >> 11) & 3;
      const auto offset5 = (instruction >>  6) & 0x1F;

      return &ARM::Thumb_LoadStoreOffsetImm<opcode, offset5>;
    }
    case ThumbInstrType::LoadStoreHword: {
      const bool load = (instruction >> 11) & 1;
      const auto offset5 = (instruction >> 6) & 0x1F;

      return &ARM::Thumb_LoadStoreHword<load, offset5>;
    }
    case ThumbInstrType::LoadStoreRelativeSP: {
      const bool load = (instruction >> 11) & 1;
      const auto rD = (instruction >> 8) & 7;

      return &ARM::Thumb_LoadStoreRelativeToSP<load, rD>;
    }
    case ThumbInstrType::LoadAddress: {
      const bool use_r13 = (instruction >> 11) & 1;
      const auto rD = (instruction >> 8) & 7;

      return &ARM::Thumb_LoadAddress<use_r13, rD>;
    }
    case ThumbInstrType::AddOffsetToSP: {
      const bool subtract = (instruction >> 7) & 1;

      return &ARM::Thumb_AddOffsetToSP<subtract>;
    }
    case ThumbInstrType::PushPop: {
      const bool load  = (instruction >> 11) & 1;
      const bool pc_lr = (instruction >>  8) & 1;

      return &ARM::Thumb_PushPop<load, pc_lr>;
    }
    case ThumbInstrType::LoadStoreMultiple: {
      const bool load = (instruction >> 11) & 1;
      const auto rB = (instruction >> 8) & 7;

      return &ARM::Thumb_LoadStoreMultiple<load, rB>;
    }
    case ThumbInstrType::ConditionalBranch: {
      const auto condition = (instruction >> 8) & 0xF;

      return &ARM::Thumb_ConditionalBranch<condition>;
    }
    case ThumbInstrType::SoftwareInterrupt: {
      return &ARM::Thumb_SWI;
    }
    case ThumbInstrType::UnconditionalBranch: {
      return &ARM::Thumb_UnconditionalBranch;
    }
    case ThumbInstrType::LongBranchLinkPrefix: {
      return &ARM::Thumb_LongBranchLinkPrefix;
    }
    case ThumbInstrType::LongBranchLinkSuffix: {
      return &ARM::Thumb_LongBranchLinkSuffix<false>;
    }
    case ThumbInstrType::LongBranchLinkExchangeSuffix: {
      return &ARM::Thumb_LongBranchLinkSuffix<true>;
    }
  }

  return &ARM::Thumb_Unimplemented;
}
