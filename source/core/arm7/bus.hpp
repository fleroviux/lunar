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

  auto GetMemoryModel() const -> MemoryModel override {
    return MemoryModel::Unprotected;
  }

  auto ReadByte(u32 address, Bus bus, int core) -> u8 override;
  auto ReadHalf(u32 address, Bus bus, int core) -> u16 override;
  auto ReadWord(u32 address, Bus bus, int core) -> u32 override;
  
  void WriteByte(u32 address, u8 value, int core) override;
  void WriteHalf(u32 address, u16 value, int core) override;
  void WriteWord(u32 address, u32 value, int core) override;

private:
  template<typename T>
  auto Read(u32 address, Bus bus, int core) -> T;

  template<typename T>
  void Write(u32 address, T value, int core);

  u8* ewram;

  // TODO: merge IWRAM with SWRAM?
  u8 iwram[0x10000];
  Interconnect::SWRAM::Alloc const& swram;
};

} // namespace fauxDS::core