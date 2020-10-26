/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <core/arm/memory.hpp>
#include <core/interconnect.hpp>

namespace fauxDS::core {

struct ARM7MemoryBus final : arm::MemoryBase {
  ARM7MemoryBus(Interconnect* interconnect);

  auto ReadByte(u32 address, Bus bus) -> u8 override;
  auto ReadHalf(u32 address, Bus bus) -> u16 override;
  auto ReadWord(u32 address, Bus bus) -> u32 override;

  void WriteByte(u32 address, u8 value) override;
  void WriteHalf(u32 address, u16 value) override;
  void WriteWord(u32 address, u32 value) override;

private:
  template<typename T>
  auto Read(u32 address, Bus bus) -> T;

  template<typename T>
  void Write(u32 address, T value);

  auto ReadByteIO(u32 address) ->  u8;
  auto ReadHalfIO(u32 address) -> u16;
  auto ReadWordIO(u32 address) -> u32;
  
  void WriteByteIO(u32 address,  u8 value);
  void WriteHalfIO(u32 address, u16 value);
  void WriteWordIO(u32 address, u32 value);

  u8* ewram;

  // TODO: merge IWRAM with SWRAM?
  u8 iwram[0x10000];
  Interconnect::SWRAM::Alloc const& swram;

  IPC& ipc;
  VideoUnit& video_unit;
  Interconnect::KeyInput& keyinput;
};

} // namespace fauxDS::core
