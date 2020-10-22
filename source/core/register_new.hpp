/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>
#include <memory>

namespace _fauxDS::core {

struct Register {
  enum class Size {
    Byte,
    Half,
    Word
  };

  virtual auto GetSize() const -> Size = 0;
  
  virtual auto ReadByte(uint offset) ->  u8 = 0;
  virtual auto ReadHalf(uint offset) -> u16 = 0;
  virtual auto ReadWord(uint offset) -> u32 = 0;

  virtual void WriteByte(uint offset,  u8 value) = 0;
  virtual void WriteHalf(uint offset, u16 value) = 0;
  virtual void WriteWord(uint offset, u32 value) = 0;
};

struct RegisterBase : Register {
  auto ReadHalf(uint offset) -> u16 override {
    return (ReadByte(offset | 0) << 0) |
           (ReadByte(offset | 1) << 8);
  }

  auto ReadWord(uint offset) -> u32 override {
    return (ReadByte(offset | 0) <<  0) |
           (ReadByte(offset | 1) <<  8) |
           (ReadByte(offset | 2) << 16) |
           (ReadByte(offset | 3) << 24);
  }

  void WriteHalf(uint offset, u16 value) override {
    WriteByte(offset | 0, value & 0xFF);
    WriteByte(offset | 1, value >> 8);
  }

  void WriteWord(uint offset, u32 value) {
    WriteByte(offset | 0, (value >>  0) & 0xFF);
    WriteByte(offset | 1, (value >>  8) & 0xFF);
    WriteByte(offset | 2, (value >> 16) & 0xFF);
    WriteByte(offset | 3, (value >> 24) & 0xFF);
  }
};

struct RegisterSet {
  RegisterSet(uint size) : table(std::make_unique<Reg32[]>(size)) {}

  void Map(Register const& reg, uint offset) {
    auto& entry = table[offset >> 2];

    switch (reg.GetSize()) {
      case Register::Size::Word: {
        entry.config = Reg32::Config::U32;
        entry.r32 = &reg;
        break;
      }
      case Register::Size::Half: {
        auto i = (offset >> 1) & 1;
        entry.config &= ~(Reg32::Config::LO_U8U8 << i);
        entry.r16[i] = &reg; 
        break;
      }
      case Register::Size::Byte: {
        
      }
    }

  }

  auto ReadByte(uint offset) -> u8 {
    return 0;
  }

  auto ReadHalf(uint offset) -> u16 {
    return 0;
  }

  auto ReadWord(uint offset) -> u32 {
    /*auto const& reg32 = table[offset >> 3];

    if (reg32.config & Reg32::Config::U32) {
      if (reg32.r32 == nullptr)
        return 0;
      return reg32.r32->ReadWord(0);
    }

    u32 value = 0;

    if (reg32.config & Reg32::Config::LO_U8U8) {
      if (reg32.r8[0] != nullptr)
        value |= reg32.r8[0]->ReadByte(0);
      if (reg32.r8[1] != nullptr)
        value |= reg32.r8[1]->ReadByte(0) << 8;
    } else {
      value |= reg32.r16[0]->ReadHalf(0);
    }

    if (reg32.config & Reg32::Config::LO_U8U8) {
      if (reg32.r8[2] != nullptr)
        value |= reg32.r8[2]->ReadByte(0) << 16;
      if (reg32.r8[3] != nullptr)
        value |= reg32.r8[3]->ReadByte(0) << 24;
    } else {
      value |= reg32.r16[1]->ReadHalf(0) << 16;
    }*/

    return 0;
  }

  void WriteByte(uint offset, u8 value) {
  }

  void WriteHalf(uint offset, u16 value) {
  }

  void WriteWord(uint offset, u32 value) {
  }

private:
  struct Reg32 {
    enum Config {
      LO_U8U8 = 1,
      HI_U8U8 = 2,
      U32 = 4
    };

    Register* r8 [4] {nullptr};
    Register* r16[2] {nullptr};
    Register* r32 {nullptr};

    int config = Config::U32;
  };

  std::unique_ptr<Reg32[]> table;
};

} // namespace fauxDS::core
