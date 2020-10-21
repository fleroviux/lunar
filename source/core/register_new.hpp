/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>

namespace fauxDS::core {

struct Register {
  virtual auto GetSize() const -> uint = 0;

  virtual auto ReadByte(uint offset) ->  u8 = 0;
  virtual auto ReadHalf(uint offset) -> u16 = 0;
  virtual auto ReadWord(uint offset) -> u32 = 0;

  virtual void WriteByte(uint offset, u8  value) = 0;
  virtual void WriteHalf(uint offset, u16 value) = 0;
  virtual void WriteWord(uint offset, u32 value) = 0;
};

struct RegisterBase : Register {
  auto ReadHalf(uint offset) -> u16 override {
    return (ReadByte(offset + 0) << 0) | 
           (ReadByte(offset + 1) << 8);
  }

  auto ReadWord(uint offset) -> u32 override {
    return (ReadByte(offset + 0) <<  0) |
           (ReadByte(offset + 1) <<  8) |
           (ReadByte(offset + 2) << 16) |
           (ReadByte(offset + 3) << 24);
  }
}

} // namespace fauxDS::core
